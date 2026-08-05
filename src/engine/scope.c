#include "engine/scope.h"

static void _scope_init(void *self, allocator_t allocator, void *arg) {
  (void)arg;
  scope_t scope = (scope_t)self;
  scope->allocator = allocator;
  scope->kind = SCOPE_GLOBAL;
  scope->parent = NULL;

  vec_init_t vec_init = {.auto_dispose = false};
  scope->children = (vec_t)allocator_create(allocator, &g_vec_type, &vec_init);

  strmap_init_t sm_init = {.value_auto_dispose = true};
  scope->names = (strmap_t)allocator_create(allocator, &g_strmap_type, &sm_init);

  vec_init_t defs_init = {.auto_dispose = true};
  scope->defs = (vec_t)allocator_create(allocator, &g_vec_type, &defs_init);

  vec_init_t defer_init = {.auto_dispose = false};
  scope->defers = (vec_t)allocator_create(allocator, &g_vec_type, &defer_init);

  scope->owner = NULL;
}

static void _scope_dispose(void *self, allocator_t allocator) {
  scope_t scope = (scope_t)self;
  /* Remove self from parent's children */
  if (scope->parent) {
    scope_remove_child(scope->parent, scope);
  }
  /* Free all children */
  while (vec_get_size(scope->children) != 0) {
    allocator_free(allocator, vec_get(scope->children, 0));
  }
  allocator_free(allocator, &scope->defs);
  allocator_free(allocator, &scope->defers);
  allocator_free(allocator, &scope->names);
  allocator_free(allocator, &scope->children);
}

type_t g_scope_type = {
    .size = sizeof(struct _scope_t),
    .name = "cubec.engine.scope",
    .init = (type_init_fn_t)_scope_init,
    .dispose = (type_dispose_fn_t)_scope_dispose,
};

scope_t scope_create(allocator_t allocator, enum scope_kind kind,
                     struct _scope_t *parent, void *owner) {
  scope_t scope = (scope_t)allocator_create(allocator, &g_scope_type, NULL);
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
