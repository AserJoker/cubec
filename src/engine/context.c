#include "engine/context.h"
#include "engine/module.h"
#include "engine/scope.h"
#include "core/string.h"
#include "core/strmap.h"
#include "cubec/program.h"
#include "cubec/token.h"
#include <stdio.h>
#include <string.h>

static void _context_init(void *self, allocator_t allocator, void *arg) {
  (void)arg;
  context_t ctx = (context_t)self;
  memset(ctx, 0, sizeof(struct context));
  ctx->allocator = allocator;

  diagnostic_list_init_t dl_init = {.output = NULL};
  ctx->diagnostics = (diagnostic_list_t)allocator_create(
      allocator, &g_diagnostic_list_type, &dl_init);

  strmap_init_t sm_init = {.value_auto_dispose = true};
  ctx->modules =
      (strmap_t)allocator_create(allocator, &g_strmap_type, &sm_init);

  ctx->global_scope = scope_create(allocator, SCOPE_GLOBAL, NULL, NULL);
  ctx->root_scope = NULL;
  ctx->current_scope = NULL;
}

static void _context_dispose(void *self, allocator_t allocator) {
  context_t ctx = (context_t)self;
  (void)allocator;
  allocator_free(allocator, &ctx->modules);
  allocator_free(allocator, &ctx->global_scope);
  allocator_free(allocator, &ctx->diagnostics);
}

type_t g_context_type = {
    .size = sizeof(struct context),
    .name = "cubec.engine.context",
    .init = (type_init_fn_t)_context_init,
    .dispose = (type_dispose_fn_t)_context_dispose,
};

context_t context_create(allocator_t allocator) {
  return (context_t)allocator_create(allocator, &g_context_type, NULL);
}

void context_dispose(context_t ctx) {
  allocator_free(ctx->allocator, &ctx);
}

int context_get_error_count(context_t ctx) {
  if (!ctx) return 0;
  return (int)diagnostic_list_get_error_count(ctx->diagnostics);
}

module_t context_get_module(context_t ctx, const char *abs_path) {
  return (module_t)strmap_find(ctx->modules, abs_path);
}

/* ------------------------------------------------------------------
 *  File I/O helper
 * -------------------------------------------------------------------------- */

static char *_read_file(allocator_t allocator, const char *path, size_t *out_len) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return NULL;
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buf = (char *)allocator_alloc(allocator, (size_t)len + 1);
  if (!buf) {
    fclose(f);
    return NULL;
  }
  size_t n = fread(buf, 1, (size_t)len, f);
  buf[n] = '\0';
  fclose(f);
  if (out_len)
    *out_len = n;
  return buf;
}

/* ------------------------------------------------------------------
 *  Path resolution
 * -------------------------------------------------------------------------- */

static char *_resolve_import_path(context_t ctx, const char *import_path) {
  if (!import_path || import_path[0] == '\0')
    return NULL;

  bool is_relative =
      (import_path[0] == '.' &&
       (import_path[1] == '/' || (import_path[1] == '.' && import_path[2] == '/')));

  bool is_absolute = (import_path[0] == '/' || import_path[0] == '\\'
#ifdef _WIN32
                      || (import_path[0] && import_path[1] == ':')
#endif
  );

  if (!is_relative && !is_absolute)
    return NULL;

  char *resolved = NULL;

  if (is_relative) {
    if (ctx->root_scope && ctx->root_scope->owner) {
      module_t current_mod = (module_t)ctx->root_scope->owner;
      const char *current_file = current_mod->filename;
      if (current_file) {
        const char *last_slash = strrchr(current_file, '/');
#ifdef _WIN32
        const char *last_backslash = strrchr(current_file, '\\');
        if (!last_slash || (last_backslash && last_backslash > last_slash))
          last_slash = last_backslash;
#endif
        if (last_slash) {
          size_t dir_len = (size_t)(last_slash - current_file) + 1;
          size_t path_len = strlen(import_path);
          resolved = (char *)allocator_alloc(ctx->allocator, dir_len + path_len + 1);
          if (!resolved)
            return NULL;
          memcpy(resolved, current_file, dir_len);
          memcpy(resolved + dir_len, import_path, path_len);
          resolved[dir_len + path_len] = '\0';
        }
      }
    }
  }

  if (!resolved) {
    size_t path_len = strlen(import_path);
    bool has_ext = (path_len > 6 && strcmp(import_path + path_len - 6, ".cubec") == 0);
    size_t ext_len = has_ext ? 0 : 6;
    resolved = (char *)allocator_alloc(ctx->allocator, path_len + ext_len + 1);
    if (!resolved)
      return NULL;
    memcpy(resolved, import_path, path_len);
    if (!has_ext) {
      memcpy(resolved + path_len, ".cubec", 6);
    }
    resolved[path_len + ext_len] = '\0';
  }

#ifdef _WIN32
  char abs_buf[_MAX_PATH];
  char *abs = _fullpath(abs_buf, resolved, _MAX_PATH);
  char *result = abs ? cstring_clone(ctx->allocator, abs) : NULL;
#else
  char real_buf[4096];
  char *real = realpath(resolved, real_buf);
  char *result = real ? cstring_clone(ctx->allocator, real) : NULL;
#endif
  allocator_free(ctx->allocator, (void **)&resolved);
  return result;
}

/* ------------------------------------------------------------------
 *  context_import
 * -------------------------------------------------------------------------- */

module_t context_import(context_t ctx, const char *import_path) {
  char *abs_path = _resolve_import_path(ctx, import_path);
  if (!abs_path)
    return NULL;

  module_t existing = context_get_module(ctx, abs_path);
  if (existing) {
    allocator_free(ctx->allocator, (void **)&abs_path);
    return existing;
  }

  char *source = _read_file(ctx->allocator, abs_path, NULL);
  if (!source) {
    allocator_free(ctx->allocator, (void **)&abs_path);
    return NULL;
  }

  vec_t tokens = resolve_token_list(ctx, abs_path, source);
  if (!tokens) {
    allocator_free(ctx->allocator, (void **)&source);
    allocator_free(ctx->allocator, (void **)&abs_path);
    return NULL;
  }

  size_t pos = 0;
  node_t program = read_program_node(ctx, tokens, &pos, abs_path);
  if (!program) {
    allocator_free(ctx->allocator, &tokens);
    allocator_free(ctx->allocator, (void **)&source);
    allocator_free(ctx->allocator, (void **)&abs_path);
    return NULL;
  }

  module_t mod = module_create(ctx->allocator, ctx->global_scope, abs_path,
                               source, tokens, program);

  strmap_insert(ctx->modules, abs_path, mod);

  allocator_free(ctx->allocator, (void **)&abs_path);
  return mod;
}

/* ------------------------------------------------------------------
 *  Scope stack
 * -------------------------------------------------------------------------- */

void context_push_scope(context_t ctx, scope_t scope) {
  if (!ctx || !scope)
    return;
  ctx->root_scope = scope;
  ctx->current_scope = scope;
}

void context_pop_scope(context_t ctx) {
  if (!ctx || !ctx->current_scope)
    return;
  ctx->current_scope = ctx->current_scope->parent;
}
