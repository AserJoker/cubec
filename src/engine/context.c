#include "engine/context.h"
#include "engine/module.h"
#include "engine/scope.h"
#include "cubec/program.h"
#include "cubec/token.h"
#include <stdio.h>
#include <stdlib.h>
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

  vec_init_t types_init = {.auto_dispose = true};
  ctx->types = (vec_t)allocator_create(allocator, &g_vec_type, &types_init);
}

static void _context_dispose(void *self, allocator_t allocator) {
  context_t ctx = (context_t)self;
  (void)allocator;
  /* Free modules first — module_dispose removes root_scope from global_scope's
   * children, so global_scope must still be alive at this point. */
  allocator_free(allocator, &ctx->modules);
  allocator_free(allocator, &ctx->types);
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
 * ------------------------------------------------------------------ */

static char *_read_file(const char *path, size_t *out_len) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return NULL;
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buf = malloc((size_t)len + 1);
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
 * ------------------------------------------------------------------ */

/**
 * @brief Resolve an import path to an absolute file path.
 *
 * - Relative paths (starting with "./" or "../") are resolved relative to the
 *   importing module's directory.
 * - Absolute paths are used as-is.
 * - Bare module names (e.g., "std") are NOT supported yet — they require
 *   package/library search paths which will be handled later.
 *
 * @param ctx         Compiler context
 * @param import_path Import path from source (e.g., "./io", "/abs/path")
 * @return Malloc'd absolute path string, or NULL on failure
 */
static char *_resolve_import_path(context_t ctx, const char *import_path) {
  if (!import_path || import_path[0] == '\0')
    return NULL;

  /* Check if path is relative (starts with ./ or ../) */
  bool is_relative =
      (import_path[0] == '.' &&
       (import_path[1] == '/' || (import_path[1] == '.' && import_path[2] == '/')));

  /* Check if path is absolute */
  bool is_absolute = (import_path[0] == '/' || import_path[0] == '\\'
#ifdef _WIN32
                      || (import_path[0] && import_path[1] == ':')
#endif
  );

  /* Bare module names (e.g., "std") are not supported */
  if (!is_relative && !is_absolute)
    return NULL;

  char *resolved = NULL;

  if (is_relative) {
    /* Resolve relative to the importing module's directory */
    if (ctx->root_scope && ctx->root_scope->owner) {
      module_t current_mod = (module_t)ctx->root_scope->owner;
      const char *current_file = current_mod->filename;
      if (current_file) {
        /* Find last '/' to get directory */
        const char *last_slash = strrchr(current_file, '/');
#ifdef _WIN32
        const char *last_backslash = strrchr(current_file, '\\');
        if (!last_slash || (last_backslash && last_backslash > last_slash))
          last_slash = last_backslash;
#endif
        if (last_slash) {
          size_t dir_len = (size_t)(last_slash - current_file) + 1;
          size_t path_len = strlen(import_path);
          resolved = malloc(dir_len + path_len + 1);
          if (!resolved)
            return NULL;
          memcpy(resolved, current_file, dir_len);
          memcpy(resolved + dir_len, import_path, path_len);
          resolved[dir_len + path_len] = '\0';
        }
      }
    }
    /* Fallback: treat as relative to cwd */
  }

  if (!resolved) {
    /* Absolute path — append .cubec extension if missing */
    size_t path_len = strlen(import_path);
    bool has_ext = (path_len > 6 && strcmp(import_path + path_len - 6, ".cubec") == 0);
    size_t ext_len = has_ext ? 0 : 6;
    resolved = malloc(path_len + ext_len + 1);
    if (!resolved)
      return NULL;
    memcpy(resolved, import_path, path_len);
    if (!has_ext) {
      memcpy(resolved + path_len, ".cubec", 6);
    }
    resolved[path_len + ext_len] = '\0';
  }

  /* Normalize to absolute path */
#ifdef _WIN32
  char abs_buf[_MAX_PATH];
  char *abs = _fullpath(abs_buf, resolved, _MAX_PATH);
  char *result = abs ? strdup(abs) : NULL;
#else
  char *result = realpath(resolved, NULL);
#endif
  free(resolved);
  return result;
}

/* ------------------------------------------------------------------
 *  context_import
 * ------------------------------------------------------------------ */

module_t context_import(context_t ctx, const char *import_path) {
  /* 1. Resolve import path to absolute file path */
  char *abs_path = _resolve_import_path(ctx, import_path);
  if (!abs_path)
    return NULL;

  /* 2. Check cache */
  module_t existing = context_get_module(ctx, abs_path);
  if (existing) {
    free(abs_path);
    return existing;
  }

  /* 3. Read source file */
  char *source = _read_file(abs_path, NULL);
  if (!source) {
    free(abs_path);
    return NULL;
  }

  /* 4. Tokenize */
  vec_t tokens = resolve_token_list(ctx, abs_path, source);
  if (!tokens) {
    free(source);
    free(abs_path);
    return NULL;
  }

  /* 5. Parse to AST */
  size_t pos = 0;
  node_t program = read_program_node(ctx, tokens, &pos, abs_path);
  if (!program) {
    allocator_free(ctx->allocator, &tokens);
    free(source);
    free(abs_path);
    return NULL;
  }

  /* 6. Create module (takes ownership of source, tokens, program) */
  module_t mod = module_create(ctx->allocator, ctx->global_scope, abs_path,
                               source, tokens, program);

  /* 7. Register in modules map (strmap with value_auto_dispose=true) */
  strmap_insert(ctx->modules, abs_path, mod);

  free(abs_path);
  return mod;
}

/* ------------------------------------------------------------------
 *  Scope stack
 * ------------------------------------------------------------------ */

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
