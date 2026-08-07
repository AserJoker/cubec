#include "engine/enum_instance.h"
#include "engine/value.h"
#include <stdlib.h>
#include <string.h>

static void _enum_instance_init(void *self, allocator_t allocator, void *arg) {
  (void)arg;
  enum_instance_t inst = (enum_instance_t)self;
  inst->instance.name = NULL;
  inst->instance.hash = 0;
  inst->instance.size = 0;
  inst->instance.align = 0;
  inst->instance.kind = TYPE_ENUM;
  inst->allocator = allocator;
  inst->variant_names = NULL;
  inst->variant_values = NULL;
  inst->underlying_type = NULL;
}

static void _enum_instance_dispose(void *self, allocator_t allocator) {
  enum_instance_t inst = (enum_instance_t)self;
  (void)allocator;
  free(inst->instance.name);
  inst->instance.name = NULL;
  if (inst->variant_names) {
    size_t n = vec_get_size(inst->variant_names);
    for (size_t i = 0; i < n; i++) {
      char *name = (char *)vec_get(inst->variant_names, i);
      free(name);
    }
    allocator_free(inst->allocator, &inst->variant_names);
  }
  if (inst->variant_values) {
    size_t n = vec_get_size(inst->variant_values);
    for (size_t i = 0; i < n; i++) {
      value_t val = (value_t)vec_get(inst->variant_values, i);
      if (val)
        allocator_free(inst->allocator, &val);
    }
    allocator_free(inst->allocator, &inst->variant_values);
  }
}

type_t g_enum_instance_type = {
    .size = sizeof(struct _enum_instance_t),
    .name = "cubec.engine.enum_instance",
    .init = (type_init_fn_t)_enum_instance_init,
    .dispose = (type_dispose_fn_t)_enum_instance_dispose,
};

enum_instance_t enum_instance_create(allocator_t allocator,
                                     const char *name,
                                     uint64_t hash,
                                     uint64_t size,
                                     uint64_t align,
                                     vec_t variant_names,
                                     vec_t variant_values,
                                     stype_t underlying_type) {
  enum_instance_t inst =
      (enum_instance_t)allocator_create(allocator, &g_enum_instance_type, NULL);
  inst->instance.name = strdup(name);
  inst->instance.hash = hash;
  inst->instance.size = size;
  inst->instance.align = align;
  inst->instance.kind = TYPE_ENUM;
  inst->variant_names = variant_names;
  inst->variant_values = variant_values;
  inst->underlying_type = underlying_type;
  return inst;
}

void enum_instance_dispose(enum_instance_t inst) {
  allocator_free(inst->allocator, &inst);
}

uint64_t enum_instance_hash_value(stype_t type, uint64_t type_hash, const void *data) {
  if (!data) return type_hash;
  /* Enum value is stored as the underlying integer type */
  uint64_t val = 0;
  memcpy(&val, data, type->instance.size);
  return stype_hash_mix_u64(type_hash, val);
}
