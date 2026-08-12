#include "engine/cfunc.h"

static void _cfunc_init(void *self, allocator_t allocator, void *arg) {
  (void)allocator;
  cfunc_t cf = (cfunc_t)self;
  cfunc_init_t *init = (cfunc_init_t *)arg;
  cf->func = init ? init->func : NULL;
}

static void _cfunc_dispose(void *self, allocator_t allocator) {
  (void)self;
  (void)allocator;
  /* func pointer does not own anything */
}

static void _cfunc_clone(void *self, allocator_t allocator, void *another) {
  (void)allocator;
  cfunc_t dst = (cfunc_t)self;
  cfunc_t src = (cfunc_t)another;
  dst->func = src->func;
}

class_t g_cfunc_class = {
    .size = sizeof(struct _cfunc_t),
    .name = "cubec.engine.cfunc",
    .init = (class_init_fn_t)_cfunc_init,
    .dispose = (class_dispose_fn_t)_cfunc_dispose,
    .clone = (class_clone_fn_t)_cfunc_clone,
    .move = NULL,
};
