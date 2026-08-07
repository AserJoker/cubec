#include "engine/stype.h"
#include <stdlib.h>
#include <string.h>

/* --------------------------------------------------------------------------
 *  FNV-1a hash utilities
 * -------------------------------------------------------------------------- */

#define FNV_OFFSET 14695981039346656037ULL
#define FNV_PRIME  1099511628211ULL

static uint64_t fnv1a_init(void) { return FNV_OFFSET; }

static uint64_t fnv1a_byte(uint64_t hash, uint8_t byte) {
  return (hash ^ byte) * FNV_PRIME;
}

static uint64_t fnv1a_u64(uint64_t hash, uint64_t val) {
  for (int i = 0; i < 8; i++)
    hash = fnv1a_byte(hash, (uint8_t)(val >> (i * 8)));
  return hash;
}

static uint64_t fnv1a_string(uint64_t hash, const char *str) {
  if (!str) return hash;
  while (*str)
    hash = fnv1a_byte(hash, (uint8_t)*str++);
  return hash;
}

uint64_t stype_compute_primitive_hash(enum type_kind_t kind) {
  return fnv1a_u64(fnv1a_init(), (uint64_t)kind);
}

uint64_t stype_compute_struct_hash(enum type_kind_t kind, vec_t field_names,
                                   vec_t field_type_hashes) {
  uint64_t h = fnv1a_u64(fnv1a_init(), (uint64_t)kind);
  size_t n = field_names ? vec_get_size(field_names) : 0;
  size_t th_n = field_type_hashes ? vec_get_size(field_type_hashes) : 0;
  h = fnv1a_u64(h, n);
  for (size_t i = 0; i < n; i++) {
    const char *name = (const char *)vec_get(field_names, i);
    h = fnv1a_string(h, name);
    if (i < th_n) {
      uint64_t th = (uint64_t)(uintptr_t)vec_get(field_type_hashes, i);
      h = fnv1a_u64(h, th);
    }
  }
  return h;
}

uint64_t stype_compute_composite_hash(enum type_kind_t kind,
                                      vec_t component_type_hashes) {
  uint64_t h = fnv1a_u64(fnv1a_init(), (uint64_t)kind);
  size_t n = component_type_hashes ? vec_get_size(component_type_hashes) : 0;
  for (size_t i = 0; i < n; i++) {
    uint64_t th = (uint64_t)(uintptr_t)vec_get(component_type_hashes, i);
    h = fnv1a_u64(h, th);
  }
  return h;
}

/* --------------------------------------------------------------------------
 *  Lifecycle
 * -------------------------------------------------------------------------- */

static void _stype_init(void *self, allocator_t allocator, void *arg) {
  (void)arg;
  stype_t type = (stype_t)self;
  type->header.allocator = allocator;
  type->header.kind = DEF_TYPE;
  type->header.node = NULL;
  type->instance.name = NULL;
  type->instance.hash = 0;
  type->instance.size = 0;
  type->instance.align = 0;
  type->type_kind = TYPE_STRUCT;
  type->params = NULL;
  type->implements = NULL;
}

static void _stype_dispose(void *self, allocator_t allocator) {
  stype_t type = (stype_t)self;
  free(type->instance.name);
  type->instance.name = NULL;
  if (type->params)
    allocator_free(allocator, &type->params);
  if (type->implements)
    allocator_free(allocator, &type->implements);
}

type_t g_stype_type = {
    .size = sizeof(struct _stype_t),
    .name = "cubec.engine.stype",
    .init = (type_init_fn_t)_stype_init,
    .dispose = (type_dispose_fn_t)_stype_dispose,
};

stype_t stype_create(allocator_t allocator, enum type_kind_t kind, node_t node) {
  stype_t type = (stype_t)allocator_create(allocator, &g_stype_type, NULL);
  type->header.kind = DEF_TYPE;
  type->header.node = node;
  type->type_kind = kind;
  return type;
}

stype_t stype_create_primitive(allocator_t allocator, enum type_kind_t kind,
                               const char *name, uint64_t size, uint64_t align) {
  stype_t type = stype_create(allocator, kind, NULL);
  type->instance.name = strdup(name);
  type->instance.hash = stype_compute_primitive_hash(kind);
  type->instance.size = size;
  type->instance.align = align;
  return type;
}

void stype_dispose(stype_t type) {
  allocator_free(type->header.allocator, &type);
}
