#include "engine/func.h"
#include "engine/scope.h"

static void _func_init(void *self, allocator_t allocator, void *arg) {
  (void)allocator;
  func_t cf = (func_t)self;
  func_init_t *init = (func_init_t *)arg;
  cf->func = init ? init->func : NULL;
  cf->name = init ? init->name : NULL;
  cf->closure_scope = init ? init->closure_scope : NULL;
  cf->root_scope = init ? init->root_scope : NULL;
}

static void _func_dispose(void *self, allocator_t allocator) {
  (void)allocator;
  func_t cf = (func_t)self;
  /* func pointer and name are borrowed references, not owned */
  if (cf->closure_scope) {
    scope_t cs = cf->closure_scope;
    cf->closure_scope = NULL;
    scope_dispose(cs);
  }
}

class_t g_func_class = {
    .size = sizeof(struct _func_t),
    .name = "cubec.engine.func",
    .init = (class_init_fn_t)_func_init,
    .dispose = (class_dispose_fn_t)_func_dispose,
    .clone = NULL,
    .move = NULL,
};

/* ---- Accessors ---- */

scope_t func_get_closure_scope(func_t self) { return self->closure_scope; }

void func_set_closure_scope(func_t self, scope_t scope) {
  self->closure_scope = scope;
}

scope_t func_get_root_scope(func_t self) { return self->root_scope; }
