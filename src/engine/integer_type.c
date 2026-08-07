#include "engine/integer_type.h"
#include "engine/name.h"
#include "engine/scope.h"
#include "core/strmap.h"
#include <string.h>

static const struct {
  enum type_kind_t kind;
  const char *name;
  uint64_t size;
  uint64_t align;
} _integer_types[] = {
    {TYPE_I8, "i8", 1, 1},   {TYPE_I16, "i16", 2, 2},
    {TYPE_I32, "i32", 4, 4}, {TYPE_I64, "i64", 8, 8},
    {TYPE_U8, "u8", 1, 1},   {TYPE_U16, "u16", 2, 2},
    {TYPE_U32, "u32", 4, 4}, {TYPE_U64, "u64", 8, 8},
};
#define NUM_INTEGER_TYPES (sizeof(_integer_types) / sizeof(_integer_types[0]))

void integer_types_register(context_t ctx) {
  allocator_t allocator = ctx->allocator;
  scope_t scope = ctx->global_scope;
  for (size_t i = 0; i < NUM_INTEGER_TYPES; i++) {
    stype_t type = stype_create_primitive(allocator, _integer_types[i].kind,
                                          _integer_types[i].name,
                                          _integer_types[i].size,
                                          _integer_types[i].align);
    rbtree_insert(ctx->types, type->instance.hash, (void *)type);
    name_t name = name_create(allocator, NAME_TYPE, (void *)type);
    strmap_insert(scope->names, _integer_types[i].name, name);
    switch (_integer_types[i].kind) {
    case TYPE_I8:  ctx->t_i8  = type; break;
    case TYPE_I16: ctx->t_i16 = type; break;
    case TYPE_I32: ctx->t_i32 = type; break;
    case TYPE_I64: ctx->t_i64 = type; break;
    case TYPE_U8:  ctx->t_u8  = type; break;
    case TYPE_U16: ctx->t_u16 = type; break;
    case TYPE_U32: ctx->t_u32 = type; break;
    case TYPE_U64: ctx->t_u64 = type; break;
    default: break;
    }
  }
}

stype_t integer_type_get(context_t ctx, enum type_kind_t kind) {
  switch (kind) {
  case TYPE_I8:  return ctx->t_i8;
  case TYPE_I16: return ctx->t_i16;
  case TYPE_I32: return ctx->t_i32;
  case TYPE_I64: return ctx->t_i64;
  case TYPE_U8:  return ctx->t_u8;
  case TYPE_U16: return ctx->t_u16;
  case TYPE_U32: return ctx->t_u32;
  case TYPE_U64: return ctx->t_u64;
  default:       return NULL;
  }
}

comptime_value_t integer_type_create_value(context_t ctx, enum type_kind_t kind, uint64_t val) {
  stype_t type = integer_type_get(ctx, kind);
  comptime_int_t v = allocator_alloc(ctx->allocator, sizeof(struct _comptime_int_t));
  v->header.allocator = ctx->allocator;
  v->header.kind = COMPTIME_VALUE_INT;
  v->header.type = type;
  v->value = val;
  return (comptime_value_t)v;
}

uint64_t integer_type_get_value(comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_INT)
    return 0;
  return ((comptime_int_t)val)->value;
}

bool type_kind_is_integer(enum type_kind_t kind) {
  return kind == TYPE_I8 || kind == TYPE_I16 || kind == TYPE_I32 ||
         kind == TYPE_I64 || kind == TYPE_U8 || kind == TYPE_U16 ||
         kind == TYPE_U32 || kind == TYPE_U64;
}
