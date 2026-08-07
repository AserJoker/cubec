#include "engine/cunion_instance.h"
#include <stdlib.h>
#include <string.h>

static void _cunion_instance_init(void *self, allocator_t allocator,
                                  void *arg) {
  (void)arg;
  cunion_instance_t inst = (cunion_instance_t)self;
  inst->instance.name = NULL;
  inst->instance.hash = 0;
  inst->instance.size = 0;
  inst->instance.align = 0;
  inst->instance.kind = TYPE_CUNION;
  inst->allocator = allocator;
  inst->fields = NULL;
}

static void _cunion_instance_dispose(void *self, allocator_t allocator) {
  cunion_instance_t inst = (cunion_instance_t)self;
  (void)allocator;
  free(inst->instance.name);
  inst->instance.name = NULL;
  if (inst->fields) {
    size_t n = vec_get_size(inst->fields);
    for (size_t i = 0; i < n; i++) {
      union_field_t f = (union_field_t)vec_get(inst->fields, i);
      union_field_dispose(f);
    }
    allocator_free(inst->allocator, &inst->fields);
  }
}

type_t g_cunion_instance_type = {
    .size = sizeof(struct _cunion_instance_t),
    .name = "cubec.engine.cunion_instance",
    .init = (type_init_fn_t)_cunion_instance_init,
    .dispose = (type_dispose_fn_t)_cunion_instance_dispose,
};

cunion_instance_t cunion_instance_create(allocator_t allocator,
                                         const char *name,
                                         uint64_t hash,
                                         uint64_t size,
                                         uint64_t align,
                                         vec_t fields) {
  cunion_instance_t inst =
      (cunion_instance_t)allocator_create(allocator, &g_cunion_instance_type,
                                          NULL);
  inst->instance.name = strdup(name);
  inst->instance.hash = hash;
  inst->instance.size = size;
  inst->instance.align = align;
  inst->instance.kind = TYPE_CUNION;
  inst->fields = fields;
  return inst;
}

void cunion_instance_dispose(cunion_instance_t inst) {
  allocator_free(inst->allocator, &inst);
}

uint64_t cunion_instance_hash_value(stype_t type, uint64_t type_hash, const void *data) {
  if (!data) return type_hash;
  /* C-union: no tag, hash all raw bytes (like C union — programmer tracks active field) */
  uint64_t h = type_hash;
  const uint8_t *bytes = (const uint8_t *)data;
  for (size_t i = 0; i < type->instance.size; i++)
    h = stype_hash_mix_u64(h, (uint64_t)bytes[i]);
  return h;
}
