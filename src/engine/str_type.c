#include "engine/str_type.h"
#include "engine/name.h"
#include "engine/scope.h"
#include "core/strmap.h"
#include "core/string.h"
#include "core/type.h"

void str_type_register(context_t ctx) {
  stype_t type = stype_create_primitive(ctx->allocator, TYPE_STR, "str", 0, 0);
  rbtree_insert(ctx->types, type->instance.hash, (void *)type);
  name_t name = name_create(ctx->allocator, NAME_TYPE, (void *)type);
  strmap_insert(ctx->global_scope->names, "str", name);
  ctx->t_str = type;
}

stype_t str_type_get(context_t ctx) {
  return ctx->t_str;
}

comptime_value_t str_type_create_value(context_t ctx, string_t val) {
  comptime_string_t v = allocator_alloc(ctx->allocator, sizeof(struct _comptime_string_t));
  v->header.allocator = ctx->allocator;
  v->header.kind = COMPTIME_VALUE_STRING;
  v->header.type = ctx->t_str;
  v->value = val;
  return (comptime_value_t)v;
}

comptime_value_t str_type_create_value_cstr(context_t ctx, const char *val) {
  string_init_t si = {.str = val};
  string_t s = allocator_create(ctx->allocator, &g_string_type, &si);
  return str_type_create_value(ctx, s);
}

string_t str_type_get_value(comptime_value_t val) {
  if (!val || val->kind != COMPTIME_VALUE_STRING)
    return NULL;
  return ((comptime_string_t)val)->value;
}
