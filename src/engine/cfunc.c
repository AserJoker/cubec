#include "engine/cfunc.h"
#include "engine/scope.h"

static void _cfunc_init(void *self, allocator_t allocator, void *arg) {
  (void)allocator;
  cfunc_t cf = (cfunc_t)self;
  cfunc_init_t *init = (cfunc_init_t *)arg;
  cf->func = init ? init->func : NULL;
  cf->name = init ? init->name : NULL;
  cf->closure_scope = init ? init->closure_scope : NULL;
  cf->root_scope = init ? init->root_scope : NULL;
}

static void _cfunc_dispose(void *self, allocator_t allocator) {
  (void)allocator;
  cfunc_t cf = (cfunc_t)self;
  /* func pointer and name are borrowed references, not owned */
  if (cf->closure_scope) {
    scope_t cs = cf->closure_scope;
    cf->closure_scope = NULL;
    scope_dispose(cs);
  }
}

static void _cfunc_clone(void *self, allocator_t allocator, void *another) {
  (void)allocator;
  cfunc_t dst = (cfunc_t)self;
  cfunc_t src = (cfunc_t)another;
  dst->func = src->func;
  dst->name = src->name;
  /* closure_scope is NOT cloned — closures are unique instances.
   * A cloned callable shares the same func/name but has no closure. */
  dst->closure_scope = NULL;
  dst->root_scope = src->root_scope;
}

class_t g_cfunc_class = {
    .size = sizeof(struct _cfunc_t),
    .name = "cubec.engine.cfunc",
    .init = (class_init_fn_t)_cfunc_init,
    .dispose = (class_dispose_fn_t)_cfunc_dispose,
    .clone = (class_clone_fn_t)_cfunc_clone,
    .move = NULL,
};

/* ---- Accessors ---- */

scope_t cfunc_get_closure_scope(cfunc_t self) { return self->closure_scope; }

void cfunc_set_closure_scope(cfunc_t self, scope_t scope) {
  self->closure_scope = scope;
}

scope_t cfunc_get_root_scope(cfunc_t self) { return self->root_scope; }
