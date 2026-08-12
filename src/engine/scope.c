#include "engine/scope.h"
#include "engine/name.h"

static void _scope_init(void *self, allocator_t allocator, void *arg) {
  (void)arg;
  scope_t scope = (scope_t)self;
  scope->allocator = allocator;
  scope->kind = SCOPE_GLOBAL;
  scope->parent = NULL;

  vec_init_t vec_init = {.auto_dispose = false};
  scope->children = (vec_t)allocator_create(allocator, &g_vec_class, &vec_init);

  strmap_init_t sm_init = {.value_auto_dispose = true};
  scope->names = (strmap_t)allocator_create(allocator, &g_strmap_class, &sm_init);

  vec_init_t val_init = {.auto_dispose = true};
  scope->values = (vec_t)allocator_create(allocator, &g_vec_class, &val_init);

  vec_init_t type_init = {.auto_dispose = true};
  scope->types = (vec_t)allocator_create(allocator, &g_vec_class, &type_init);

  vec_init_t str_init = {.auto_dispose = true};
  scope->strings = (vec_t)allocator_create(allocator, &g_vec_class, &str_init);

  vec_init_t cfunc_init = {.auto_dispose = true};
  scope->cfuncs = (vec_t)allocator_create(allocator, &g_vec_class, &cfunc_init);

  vec_init_t defer_init = {.auto_dispose = false};
  scope->defers = (vec_t)allocator_create(allocator, &g_vec_class, &defer_init);

  scope->owner = NULL;
}

static void _scope_dispose(void *self, allocator_t allocator) {
  scope_t scope = (scope_t)self;
  /* Remove self from parent's children */
  if (scope->parent) {
    scope_remove_child(scope->parent, scope);
  }
  /* Free all children — iterate and free each, then destroy the vec.
   * children vec is auto_dispose=false, so we manage child lifecycle manually.
   * Clear parent pointer first to prevent double-remove from parent's vec. */
  size_t n = vec_get_size(scope->children);
  for (size_t i = 0; i < n; i++) {
    scope_t child = (scope_t)vec_get(scope->children, i);
    child->parent = NULL;
    allocator_free(allocator, &child);
  }
  allocator_free(allocator, &scope->defers);
  allocator_free(allocator, &scope->values);
  allocator_free(allocator, &scope->cfuncs);
  allocator_free(allocator, &scope->strings);
  allocator_free(allocator, &scope->types);
  allocator_free(allocator, &scope->names);
  allocator_free(allocator, &scope->children);
}

class_t g_scope_class = {
    .size = sizeof(struct _scope_t),
    .name = "cubec.engine.scope",
    .init = (class_init_fn_t)_scope_init,
    .dispose = (class_dispose_fn_t)_scope_dispose,
};

scope_t scope_create(allocator_t allocator, enum scope_kind kind,
                     struct _scope_t *parent, void *owner) {
  scope_t scope = (scope_t)allocator_create(allocator, &g_scope_class, NULL);
  scope->kind = kind;
  scope->parent = parent;
  scope->owner = owner;
  /* Add self to parent's children */
  scope_add_child(parent, scope);
  return scope;
}

void scope_add_child(struct _scope_t *parent, scope_t child) {
  if (parent && child) {
    vec_push(parent->children, child);
  }
}

void scope_remove_child(struct _scope_t *parent, scope_t child) {
  if (!parent)
    return;
  size_t size = vec_get_size(parent->children);
  for (size_t i = 0; i < size; i++) {
    if (vec_get(parent->children, i) == child) {
      vec_remove(parent->children, i);
      return;
    }
  }
}

void scope_dispose(scope_t scope) {
  allocator_free(scope->allocator, &scope);
}

name_t scope_lookup(scope_t scope, const char *name_str) {
  if (!scope || !name_str)
    return NULL;
  for (scope_t s = scope; s; s = s->parent) {
    name_t name = (name_t)strmap_find(s->names, name_str);
    if (name)
      return name;
  }
  return NULL;
}
