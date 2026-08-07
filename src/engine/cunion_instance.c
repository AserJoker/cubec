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
  inst->fields = fields;
  return inst;
}

void cunion_instance_dispose(cunion_instance_t inst) {
  allocator_free(inst->allocator, &inst);
}

/* ---- C-union comptime value operations (no tag — like C union) ---- */

void cunion_instance_dispose_value(comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_CUNION) return;
  comptime_cunion_t v = (comptime_cunion_t)val;
  if (v->value)
    comptime_value_dispose(v->value);
  allocator_free(val->allocator, &val);
}

comptime_value_t cunion_instance_clone_value(allocator_t allocator,
                                             comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_CUNION) return NULL;
  comptime_cunion_t src = (comptime_cunion_t)val;
  comptime_cunion_t dst = allocator_alloc(allocator, sizeof(struct _comptime_cunion_t));
  dst->header = src->header;
  dst->header.allocator = allocator;
  dst->value = src->value ? comptime_value_clone(allocator, src->value) : NULL;
  return (comptime_value_t)dst;
}

uint64_t cunion_instance_hash_value(comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_CUNION) return 0;
  comptime_cunion_t v = (comptime_cunion_t)val;
  uint64_t h = stype_compute_primitive_hash(TYPE_CUNION);
  if (val->type)
    h = stype_hash_mix_u64(h, val->type->instance.hash);
  h = stype_hash_mix_u64(h, comptime_value_hash(v->value));
  return h;
}
