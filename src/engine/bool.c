#include "engine/bool.h"
#include "core/allocator.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/float.h"
#include "engine/integer.h"
#include "engine/type.h"
#include "engine/unsigned.h"
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
  if (value_is_comptime(self)) {
    bool val = *(bool *)value_get_data(self);
    if (type_get_kind(type) == TYPE_KIND_INTEGER) {
      return create_comptime_integer(ctx, type, val ? 1 : 0);
    } else if (type_get_kind(type) == TYPE_KIND_UNSIGNED) {
      return create_comptime_unsigned(ctx, type, val ? 1 : 0);
    } else if (type_get_kind(type) == TYPE_KIND_FLOAT) {
      return create_comptime_float(ctx, type, val ? 1 : 0);
    }
  } else {
    if (type_get_kind(type) == TYPE_KIND_INTEGER) {
      return create_integer(ctx, type);
    } else if (type_get_kind(type) == TYPE_KIND_UNSIGNED) {
      return create_unsigned(ctx, type);
    } else if (type_get_kind(type) == TYPE_KIND_FLOAT) {
      return create_float(ctx, type);
    }
  }
  return create_error(ctx, "cannot convert 'bool' to '%s'",
                      type_get_name(type));
}
static value_t bool_eq(value_t self, context_t ctx, value_t another) {
  type_t type = value_get_type(another);
  type_t bool_t = value_get_type(self);
  if (type_get_kind(type) != TYPE_KIND_BOOL) {
    another = value_safe_convert(another, ctx, bool_t);
    if (value_is_error(another)) {
      return another;
    }
  }
  if (value_is_comptime(self) && value_is_comptime(another)) {
    bool left = *(bool *)value_get_data(self);
    bool right = *(bool *)value_get_data(another);
    return create_comptime_bool(ctx, left == right, false, NULL);
  }
  return create_bool(ctx, false, NULL);
}
static value_t bool_ne(value_t self, context_t ctx, value_t another) {
  type_t type = value_get_type(another);
  type_t bool_t = value_get_type(self);
  if (type_get_kind(type) != TYPE_KIND_BOOL) {
    another = value_safe_convert(another, ctx, bool_t);
    if (value_is_error(another)) {
      return another;
    }
  }
  if (value_is_comptime(self) && value_is_comptime(another)) {
    bool left = *(bool *)value_get_data(self);
    bool right = *(bool *)value_get_data(another);
    return create_comptime_bool(ctx, left != right, false, NULL);
  }
  return create_bool(ctx, false, NULL);
}
void bool_init(context_t ctx) {
  allocator_t allocator = context_get_allocator(ctx);
  type_operator_t opt = {
      .type_eq = type_default_eq,
      .addr_of = value_default_address_of,
      .logical_not = bool_logical_not,
      .logical_and = bool_logical_and,
      .logical_or = bool_logical_or,
      .assigment = value_default_assigment,
      .convert = bool_convert,
      .eq = bool_eq,
      .ne = bool_ne,
  };
  type_t bool_t = create_type(allocator, TYPE_KIND_BOOL, sizeof(bool),
                              sizeof(bool), "bool", "bool", &opt, NULL);
  context_store_type(ctx, bool_t);
  create_type_value(ctx, bool_t, false, "bool");
}
value_t create_comptime_bool(context_t ctx, bool value, bool mut,
                             const char *name) {
  allocator_t allocator = context_get_allocator(ctx);
  value_t bool_v = context_load(ctx, "bool");
  type_t bool_t = *(type_t *)value_get_data(bool_v);
  return context_create_value(ctx, bool_t, &value, mut, true, name);
}

value_t create_bool(context_t ctx, bool mut, const char *name) {
  allocator_t allocator = context_get_allocator(ctx);
  value_t bool_v = context_load(ctx, "bool");
  type_t bool_t = *(type_t *)value_get_data(bool_v);
  return context_create_value(ctx, bool_t, NULL, mut, false, name);
}
bool bool_get_value(value_t val) {
  bool *data = value_get_data(val);
  return *data;
}