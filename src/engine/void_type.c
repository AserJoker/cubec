#include "engine/void_type.h"
#include "engine/name.h"
#include "engine/scope.h"
#include "core/strmap.h"

void void_type_register(context_t ctx) {
  stype_t type = stype_create_primitive(ctx->allocator, TYPE_VOID, "void", 0, 0);
  rbtree_insert(ctx->types, type->instance.hash, (void *)type);
  name_t name = name_create(ctx->allocator, NAME_TYPE, (void *)type);
  strmap_insert(ctx->global_scope->names, "void", name);
  ctx->t_void = type;
}

stype_t void_type_get(context_t ctx) {
  return ctx->t_void;
}

uint64_t void_type_hash_value(stype_t type, uint64_t type_hash, const void *data) {
  (void)type;
  (void)data;
  return type_hash;
}
