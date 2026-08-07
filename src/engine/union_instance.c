#include "engine/union_instance.h"
#include "engine/context.h"
#include "core/rbtree.h"
#include <stdlib.h>
#include <string.h>

static void _union_instance_init(void *self, allocator_t allocator,
                                 void *arg) {
  (void)arg;
  union_instance_t inst = (union_instance_t)self;
  inst->instance.name = NULL;
  inst->instance.hash = 0;
  inst->instance.size = 0;
  inst->instance.align = 0;
  inst->instance.kind = TYPE_UNION;
  inst->allocator = allocator;
  inst->fields = NULL;
  inst->members = NULL;
  inst->methods = NULL;
}

static void _union_instance_dispose(void *self, allocator_t allocator) {
  union_instance_t inst = (union_instance_t)self;
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
  if (inst->members)
    allocator_free(inst->allocator, &inst->members);
  if (inst->methods)
    allocator_free(inst->allocator, &inst->methods);
}

type_t g_union_instance_type = {
    .size = sizeof(struct _union_instance_t),
    .name = "cubec.engine.union_instance",
    .init = (type_init_fn_t)_union_instance_init,
    .dispose = (type_dispose_fn_t)_union_instance_dispose,
};

union_instance_t union_instance_create(allocator_t allocator,
                                       const char *name,
                                       uint64_t hash,
                                       uint64_t size,
                                       uint64_t align,
                                       vec_t fields,
                                       strmap_t members,
                                       strmap_t methods) {
  union_instance_t inst =
      (union_instance_t)allocator_create(allocator, &g_union_instance_type,
                                         NULL);
  inst->instance.name = strdup(name);
  inst->instance.hash = hash;
  inst->instance.size = size;
  inst->instance.align = align;
  inst->instance.kind = TYPE_UNION;
  inst->fields = fields;
  inst->members = members;
  inst->methods = methods;
  return inst;
}

void union_instance_dispose(union_instance_t inst) {
  allocator_free(inst->allocator, &inst);
}

uint64_t union_instance_hash_value(context_t ctx, stype_t type,
                                   uint64_t type_hash, const void *data) {
  if (!data) return type_hash;
  union_instance_t inst =
      (union_instance_t)rbtree_find(type->implements, type_hash);
  uint64_t h = type_hash;
  if (inst && inst->fields) {
    /* First sizeof(uint64_t) bytes is the tag (active variant index) */
    uint64_t tag;
    memcpy(&tag, data, sizeof(uint64_t));
    h = stype_hash_mix_u64(h, tag);
    /* Hash the active variant data */
    if (tag < vec_get_size(inst->fields)) {
      union_field_t f = (union_field_t)vec_get(inst->fields, tag);
      /* Variant data starts after the tag, aligned to max(align, sizeof(uint64_t)) */
      size_t data_offset = type->instance.align > sizeof(uint64_t)
                               ? type->instance.align
                               : sizeof(uint64_t);
      h = stype_hash_mix_u64(h,
          value_data_hash(ctx, f->type, f->type->instance.hash,
                          (const char *)data + data_offset));
    }
  }
  return h;
}
