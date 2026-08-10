#include "engine/stype.h"
#include <string.h>

static void _value_init(void *self, allocator_t allocator, void *arg) {
  (void)allocator;
  value_t *v = (value_t *)self;
  value_init_t *init = (value_init_t *)arg;
  v->type = init->type;
  v->data = init->data;
  v->own = init->own;
}

static void _value_dispose(void *self, allocator_t allocator) {
  value_t *v = (value_t *)self;
  if (v->own && v->data && v->type->vtable.dispose) {
    v->type->vtable.dispose(allocator, v);
  }
  v->type = NULL;
  v->data = NULL;
  v->own = false;
}

static void _value_clone(void *self, allocator_t allocator, void *another) {
  value_t *dst = (value_t *)self;
  value_t *src = (value_t *)another;
  dst->type = src->type;
  dst->own = false;
  dst->data = NULL;
  if (src->type->vtable.clone) {
    value_t *cloned = src->type->vtable.clone(allocator, src);
    dst->data = cloned->data;
    dst->own = cloned->own;
    /* cloned value_t shell is temporary — free the shell only */
    cloned->own = false;
    cloned->data = NULL;
    allocator_free(allocator, &cloned);
  }
}

static void _value_move(void *self, allocator_t allocator, void *another) {
  (void)allocator;
  value_t *dst = (value_t *)self;
  value_t *src = (value_t *)another;
  dst->type = src->type;
  dst->data = src->data;
  dst->own = src->own;
  src->data = NULL;
  src->own = false;
}

class_t g_value_class = {
    .size = sizeof(value_t),
    .name = "cubec.engine.value",
    .init = (class_init_fn_t)_value_init,
    .dispose = (class_dispose_fn_t)_value_dispose,
    .clone = (class_clone_fn_t)_value_clone,
    .move = (class_move_fn_t)_value_move,
};

value_t *value_create(allocator_t allocator, stype_t *type, void *data,
                      bool own) {
  value_init_t init = {.type = type, .data = data, .own = own};
  return (value_t *)allocator_create(allocator, &g_value_class, &init);
}

void value_dispose(value_t *self, allocator_t allocator) {
  if (!self) return;
  allocator_free(allocator, &self);
}

stype_t *value_get_type(const value_t *self) { return self->type; }
void    *value_get_data(const value_t *self) { return self->data; }
bool     value_is_own(const value_t *self) { return self->own; }
bool     value_is_shadow(const value_t *self) { return self->data == NULL; }
