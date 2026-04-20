#include "engine/bool.h"
#include "core/allocator.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdbool.h>
static value_t bool_logical_not(value_t self, context_t ctx) {
  if (value_is_comptime(self)) {
    const bool *value = value_get_data(self);
    return create_comptime_bool(ctx, !*value, false, NULL);
  } else {
    return create_bool(ctx, false, NULL);
  }
}
static value_t bool_logical_and(value_t self, context_t ctx, value_t another) {
  type_t right_type = value_get_type(another);
  if (type_get_kind(right_type) != TYPE_KIND_BOOL) {
    type_t type = value_get_type(self);
    return create_error(ctx,
                        "invalid operands to binary expression ('%s' and '%s')",
                        type_get_name(type), type_get_name(right_type));
  }
  if (value_is_comptime(self) && value_is_comptime(another)) {
    bool left = *(bool *)value_get_data(self);
    bool right = *(bool *)value_get_data(another);
    return create_comptime_bool(ctx, left && right, false, NULL);
  } else {
    return create_bool(ctx, false, NULL);
  }
}
static value_t bool_logical_or(value_t self, context_t ctx, value_t another) {
  type_t right_type = value_get_type(another);
  if (type_get_kind(right_type) != TYPE_KIND_BOOL) {
    type_t type = value_get_type(self);
    return create_error(ctx,
                        "invalid operands to binary expression ('%s' and '%s')",
                        type_get_name(type), type_get_name(right_type));
  }
  if (value_is_comptime(self) && value_is_comptime(another)) {
    bool left = *(bool *)value_get_data(self);
    bool right = *(bool *)value_get_data(another);
    return create_comptime_bool(ctx, left || right, false, NULL);
  } else {
    return create_bool(ctx, false, NULL);
  }
}
static value_t bool_convert(value_t self, context_t ctx, type_t type) {
  if (type_get_kind(type) == TYPE_KIND_INTEGER) {

  } else if (type_get_kind(type) == TYPE_KIND_UNSIGNED) {

  } else if (type_get_kind(type) == TYPE_KIND_FLOAT) {
  }
  return create_error(ctx, "cannot convert 'bool' to '%s'",
                      type_get_name(type));
}
void bool_init(context_t ctx) {
  allocator_t allocator = context_get_allocator(ctx);
  type_operator_t opt = {
      .logical_not = bool_logical_not,
      .logical_and = bool_logical_and,
      .logical_or = bool_logical_or,
  };
  type_t bool_t = create_type(allocator, TYPE_KIND_BOOL, sizeof(bool),
                              sizeof(bool), "bool", "bool", &opt, NULL);
  context_store_type(ctx, bool_t);
  create_type_value(ctx, bool_t, false, true, "bool");
  create_comptime_bool(ctx, true, false, "true");
  create_comptime_bool(ctx, false, false, "false");
}
value_t create_comptime_bool(context_t ctx, bool value, bool mut,
                             const char *name) {
  allocator_t allocator = context_get_allocator(ctx);
  value_t bool_v = context_load(ctx, "bool");
  type_t bool_t = (type_t)value_get_data(bool_v);
  return context_create_value(ctx, bool_t, &value, mut, true, name);
}

value_t create_bool(context_t ctx, bool mut, const char *name) {
  allocator_t allocator = context_get_allocator(ctx);
  value_t bool_v = context_load(ctx, "bool");
  type_t bool_t = (type_t)value_get_data(bool_v);
  return context_create_value(ctx, bool_t, NULL, mut, false, name);
}