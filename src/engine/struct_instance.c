#include "engine/struct_instance.h"
#include "core/strmap.h"
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
  inst->fields = NULL;
  inst->members = NULL;
  inst->methods = NULL;
}

static void _struct_instance_dispose(void *self, allocator_t allocator) {
  struct_instance_t inst = (struct_instance_t)self;
  (void)allocator;
  free(inst->instance.name);
  inst->instance.name = NULL;
  if (inst->fields) {
    size_t n = vec_get_size(inst->fields);
    for (size_t i = 0; i < n; i++) {
      struct_field_t f = (struct_field_t)vec_get(inst->fields, i);
      struct_field_dispose(f);
    }
    allocator_free(inst->allocator, &inst->fields);
  }
  if (inst->members)
    allocator_free(inst->allocator, &inst->members);
  if (inst->methods)
    allocator_free(inst->allocator, &inst->methods);
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
                                         vec_t fields,
                                         strmap_t members,
                                         strmap_t methods) {
  struct_instance_t inst =
      (struct_instance_t)allocator_create(allocator, &g_struct_instance_type,
                                          NULL);
  inst->instance.name = strdup(name);
  inst->instance.hash = hash;
  inst->instance.size = size;
  inst->instance.align = align;
  inst->fields = fields;
  inst->members = members;
  inst->methods = methods;
  return inst;
}

void struct_instance_dispose(struct_instance_t inst) {
  allocator_free(inst->allocator, &inst);
}

/* ---- Struct comptime value operations ---- */

static void _dispose_value_vec(vec_t vec, allocator_t allocator) {
  if (!vec) return;
  size_t n = vec_get_size(vec);
  for (size_t i = 0; i < n; i++)
    comptime_value_dispose((comptime_value_t)vec_get(vec, i));
  allocator_free(allocator, &vec);
}

static vec_t _clone_value_vec(allocator_t allocator, vec_t src) {
  if (!src) return NULL;
  vec_init_t vi = {.auto_dispose = true};
  vec_t dst = (vec_t)allocator_create(allocator, &g_vec_type, &vi);
  size_t n = vec_get_size(src);
  for (size_t i = 0; i < n; i++) {
    comptime_value_t f = comptime_value_clone(allocator, vec_get(src, i));
    vec_push(dst, f);
  }
  return dst;
}

static uint64_t _hash_value_vec(vec_t vec) {
  if (!vec) return 0;
  uint64_t h = 0;
  size_t n = vec_get_size(vec);
  h = stype_hash_mix_u64(h, n);
  for (size_t i = 0; i < n; i++) {
    comptime_value_t f = (comptime_value_t)vec_get(vec, i);
    h = stype_hash_mix_u64(h, comptime_value_hash(f));
  }
  return h;
}

void struct_instance_dispose_value(comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_STRUCT) return;
  comptime_struct_t v = (comptime_struct_t)val;
  _dispose_value_vec(v->fields, val->allocator);
  allocator_free(val->allocator, &val);
}

comptime_value_t struct_instance_clone_value(allocator_t allocator,
                                             comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_STRUCT) return NULL;
  comptime_struct_t src = (comptime_struct_t)val;
  comptime_struct_t dst = allocator_alloc(allocator, sizeof(struct _comptime_struct_t));
  dst->header = src->header;
  dst->header.allocator = allocator;
  dst->fields = _clone_value_vec(allocator, src->fields);
  return (comptime_value_t)dst;
}

uint64_t struct_instance_hash_value(comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_STRUCT) return 0;
  comptime_struct_t v = (comptime_struct_t)val;
  uint64_t h = stype_compute_primitive_hash(TYPE_STRUCT);
  if (val->type)
    h = stype_hash_mix_u64(h, val->type->instance.hash);
  h = stype_hash_mix_u64(h, _hash_value_vec(v->fields));
  return h;
}
