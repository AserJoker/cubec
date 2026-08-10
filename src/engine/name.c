#include "engine/name.h"

static void _name_init(void *self, allocator_t allocator, void *arg) {
  (void)arg;
  name_t name = (name_t)self;
  name->allocator = allocator;
  name->ref = NULL;
}

static void _name_dispose(void *self, allocator_t allocator) {
  (void)self;
  (void)allocator;
}

class_t g_name_class = {
    .size = sizeof(struct _name_t),
    .name = "cubec.engine.name",
    .init = (class_init_fn_t)_name_init,
    .dispose = (class_dispose_fn_t)_name_dispose,
};

name_t name_create(allocator_t allocator, value_t ref) {
  name_t name = (name_t)allocator_create(allocator, &g_name_class, NULL);
  name->ref = ref;
  return name;
}

void name_dispose(name_t name) {
  allocator_free(name->allocator, &name);
}
