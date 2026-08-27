#include "engine/defer.h"
#include "engine/scope.h"

static void _defer_init(void *self, allocator_t allocator, void *arg) {
  (void)allocator;
  defer_t d = (defer_t)self;
  defer_init_t *init = (defer_init_t *)arg;
  d->func = init ? init->func : NULL;
  d->closure_scope = init ? init->closure_scope : NULL;
  d->root_scope = init ? init->root_scope : NULL;
}

static void _defer_dispose(void *self, allocator_t allocator) {
  (void)allocator;
  defer_t d = (defer_t)self;
  if (d->closure_scope) {
    scope_t cs = d->closure_scope;
    d->closure_scope = NULL;
    scope_dispose(cs);
  }
}

class_t g_defer_class = {
    .size = sizeof(struct _defer_t),
    .name = "cubec.engine.defer",
    .init = (class_init_fn_t)_defer_init,
    .dispose = (class_dispose_fn_t)_defer_dispose,
    .clone = NULL,
    .move = NULL,
};
