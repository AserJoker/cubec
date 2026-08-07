#include "engine/struct_instance.h"
#include <stdlib.h>
#include <string.h>

static void _struct_instance_init(void *self, allocator_t allocator,
                                  void *arg) {
  (void)arg;
  struct_instance_t inst = (struct_instance_t)self;
  inst->instance.name = NULL;
  inst->instance.hash = 0;
  inst->instance.size = 0;
  inst->instance.align = 0;
  inst->allocator = allocator;
  inst->field_names = NULL;
  inst->field_types = NULL;
  inst->field_offsets = NULL;
}

static void _struct_instance_dispose(void *self, allocator_t allocator) {
  struct_instance_t inst = (struct_instance_t)self;
  (void)allocator;
  free(inst->instance.name);
  inst->instance.name = NULL;
  if (inst->field_names) {
    size_t n = vec_get_size(inst->field_names);
    for (size_t i = 0; i < n; i++) {
      char *name = (char *)vec_get(inst->field_names, i);
      free(name);
    }
    allocator_free(inst->allocator, &inst->field_names);
  }
  if (inst->field_types)
    allocator_free(inst->allocator, &inst->field_types);
  if (inst->field_offsets)
    allocator_free(inst->allocator, &inst->field_offsets);
}

type_t g_struct_instance_type = {
    .size = sizeof(struct _struct_instance_t),
    .name = "cubec.engine.struct_instance",
    .init = (type_init_fn_t)_struct_instance_init,
    .dispose = (type_dispose_fn_t)_struct_instance_dispose,
};

struct_instance_t struct_instance_create(allocator_t allocator,
                                         const char *name,
                                         uint64_t hash,
                                         uint64_t size,
                                         uint64_t align,
                                         vec_t field_names,
                                         vec_t field_types,
                                         vec_t field_offsets) {
  struct_instance_t inst =
      (struct_instance_t)allocator_create(allocator, &g_struct_instance_type,
                                          NULL);
  inst->instance.name = strdup(name);
  inst->instance.hash = hash;
  inst->instance.size = size;
  inst->instance.align = align;
  inst->field_names = field_names;
  inst->field_types = field_types;
  inst->field_offsets = field_offsets;
  return inst;
}

void struct_instance_dispose(struct_instance_t inst) {
  allocator_free(inst->allocator, &inst);
}
