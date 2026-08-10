#include "engine/vm.h"
#include "engine/context.h"
#include "engine/scope.h"
#include "engine/type.h"
#include "engine/module.h"
#include "core/string.h"
#include "core/strmap.h"
#include "cubec/program.h"
#include "cubec/token.h"
#include <stdio.h>
#include <string.h>

struct _vm_t {
  allocator_t allocator;
  strmap_t    modules;       /* absolute path → module_t (auto-dispose) */
  scope_t     global_scope;  /* owned: global scope */
  scope_t     root_scope;    /* borrowed: current module's root scope */
  scope_t     current_scope; /* borrowed: current traversal position */
  value_t     v_type;        /* borrowed: bootstrap type "type" (in global_scope->values) */
};

static void _vm_init(void *self, allocator_t allocator, void *arg) {
  (void)arg;
  vm_t vm = (vm_t)self;
  vm->allocator = allocator;

  strmap_init_t sm_init = {.value_auto_dispose = true};
  vm->modules = (strmap_t)allocator_create(allocator, &g_strmap_class, &sm_init);

  vm->global_scope = scope_create(allocator, SCOPE_GLOBAL, NULL, NULL);
  vm->root_scope = NULL;
  vm->current_scope = NULL;

  /* Bootstrap: create the "type" type */
  type_t type_type = type_create_type_type(allocator);

  /* v_type: value where type=ref, data=own, both point to the same type_t */
  vm->v_type = value_create(allocator, type_type, type_type, true);
  vec_push(vm->global_scope->values, vm->v_type);
}

static void _vm_dispose(void *self, allocator_t allocator) {
  vm_t vm = (vm_t)self;
  (void)allocator;
  allocator_free(vm->allocator, &vm->modules);
  allocator_free(vm->allocator, &vm->global_scope);
}

class_t g_vm_class = {
    .size = sizeof(struct _vm_t),
    .name = "cubec.engine.vm",
    .init = (class_init_fn_t)_vm_init,
    .dispose = (class_dispose_fn_t)_vm_dispose,
    .clone = NULL,
    .move = NULL,
};

vm_t vm_create(allocator_t allocator) {
  return (vm_t)allocator_create(allocator, &g_vm_class, NULL);
}

void vm_dispose(vm_t self, allocator_t allocator) {
  if (!self) return;
  allocator_free(allocator, &self);
}

allocator_t vm_get_allocator(vm_t self) { return self->allocator; }
strmap_t vm_get_modules(vm_t self) { return self->modules; }
scope_t  vm_get_global_scope(vm_t self) { return self->global_scope; }
scope_t  vm_get_root_scope(vm_t self) { return self->root_scope; }
scope_t  vm_get_current_scope(vm_t self) { return self->current_scope; }
value_t  vm_get_type_type(vm_t self) { return self->v_type; }

module_t vm_get_module(vm_t self, const char *abs_path) {
  return (module_t)strmap_find(self->modules, abs_path);
}

/* ---- File I/O ---- */

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

/* ---- Path resolution ---- */

static char *_resolve_import_path(vm_t vm, const char *import_path) {
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
    if (vm->root_scope && vm->root_scope->owner) {
      module_t current_mod = (module_t)vm->root_scope->owner;
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
          resolved = (char *)allocator_alloc(vm->allocator, dir_len + path_len + 1);
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
    resolved = (char *)allocator_alloc(vm->allocator, path_len + ext_len + 1);
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
  char *result = abs ? cstring_clone(vm->allocator, abs) : NULL;
#else
  char real_buf[4096];
  char *real = realpath(resolved, real_buf);
  char *result = real ? cstring_clone(vm->allocator, real) : NULL;
#endif
  allocator_free(vm->allocator, (void **)&resolved);
  return result;
}

/* ---- vm_import ---- */

module_t vm_import(vm_t self, context_t ctx, const char *import_path) {
  char *abs_path = _resolve_import_path(self, import_path);
  if (!abs_path)
    return NULL;

  module_t existing = vm_get_module(self, abs_path);
  if (existing) {
    allocator_free(self->allocator, (void **)&abs_path);
    return existing;
  }

  char *source = _read_file(self->allocator, abs_path, NULL);
  if (!source) {
    allocator_free(self->allocator, (void **)&abs_path);
    return NULL;
  }

  vec_t tokens = resolve_token_list(ctx, abs_path, source);
  if (!tokens) {
    allocator_free(self->allocator, (void **)&source);
    allocator_free(self->allocator, (void **)&abs_path);
    return NULL;
  }

  size_t pos = 0;
  node_t program = read_program_node(ctx, tokens, &pos, abs_path);
  if (!program) {
    allocator_free(self->allocator, &tokens);
    allocator_free(self->allocator, (void **)&source);
    allocator_free(self->allocator, (void **)&abs_path);
    return NULL;
  }

  module_t mod = module_create(self->allocator, self->global_scope, abs_path,
                               source, tokens, program);

  strmap_insert(self->modules, abs_path, mod);

  allocator_free(self->allocator, (void **)&abs_path);
  return mod;
}

/* ---- Scope stack ---- */

void vm_push_scope(vm_t self, scope_t scope) {
  if (!self || !scope)
    return;
  self->root_scope = scope;
  self->current_scope = scope;
}

void vm_pop_scope(vm_t self) {
  if (!self || !self->current_scope)
    return;
  self->current_scope = self->current_scope->parent;
}

/* ---- Value creation ---- */

value_t vm_create_value(vm_t self, type_t type, const void *data,
                        const char *name) {
  size_t sz = type_get_size(type);
  void *data_copy = NULL;
  if (sz > 0) {
    data_copy = allocator_alloc(self->allocator, sz);
    if (data) {
      memcpy(data_copy, data, sz);
    } else {
      memset(data_copy, 0, sz);
    }
  }
  value_t v = value_create(self->allocator, type, data_copy, true);
  if (self->current_scope) {
    vec_push(self->current_scope->values, v);
    if (name) {
      name_t n = name_create(self->current_scope->allocator, v);
      char *owned_name = cstring_clone(self->current_scope->allocator, name);
      strmap_insert(self->current_scope->names, owned_name, n);
      allocator_free(self->current_scope->allocator, &owned_name);
    }
  }
  return v;
}

value_t vm_create_value_shadow(vm_t self, type_t type, const char *name) {
  value_t v = value_create(self->allocator, type, NULL, false);
  if (self->current_scope) {
    vec_push(self->current_scope->values, v);
    if (name) {
      name_t n = name_create(self->current_scope->allocator, v);
      char *owned_name = cstring_clone(self->current_scope->allocator, name);
      strmap_insert(self->current_scope->names, owned_name, n);
      allocator_free(self->current_scope->allocator, &owned_name);
    }
  }
  return v;
}
