#include "engine/boolean.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/numeric.h"
#include "engine/str.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdbool.h>

static char *boolean_type_to_string(type_t self, allocator_t allocator) {
  return create_cstring(allocator, "bool");
}

static value_t boolean_to_string(value_t value, context_t ctx) {
  bool val = *(bool *)value_get_data(value);
  const char *str = val ? "true" : "false";
  return create_str(ctx, str, NULL);
}
static value_t boolean_logical_and(value_t self, context_t ctx,
                                   value_t another) {
  bool *left = (bool *)value_get_data(self);
  bool *right = (bool *)value_get_data(another);
  if (!left || !right) {
    value_t vtype = context_load(ctx, "bool");
    type_t type = *(type_t *)value_get_data(vtype);
    return context_create_value(ctx, type, false, NULL, NULL);
  }
  return create_boolean(ctx, (*left) && (*right), false, NULL);
}
static value_t boolean_logical_or(value_t self, context_t ctx,
                                  value_t another) {
  bool *left = (bool *)value_get_data(self);
  bool *right = (bool *)value_get_data(another);
  if (!left || !right) {
    value_t vtype = context_load(ctx, "bool");
    type_t type = *(type_t *)value_get_data(vtype);
    return context_create_value(ctx, type, false, NULL, NULL);
  }
  return create_boolean(ctx, (*left) || (*right), false, NULL);
}
static value_t boolean_logical_not(value_t self, context_t ctx) {
  bool *data = (bool *)value_get_data(self);
  if (!data) {
    value_t vtype = context_load(ctx, "bool");
    type_t type = *(type_t *)value_get_data(vtype);
    return context_create_value(ctx, type, false, NULL, NULL);
  }
  return create_boolean(ctx, !(*data), false, NULL);
}
static value_t boolean_convert(value_t value, context_t ctx, type_t type) {
  bool *data = value_get_data(value);
  type_kind_t kind = type_get_kind(type);
  switch (kind) {
  case CUBEC_VALUE_TYPE_BOOL:
    if (data) {
      return create_boolean(ctx, *data, true, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL, NULL);
    }
  case CUBEC_VALUE_TYPE_INT8:
    if (data) {
      return create_i8(ctx, *data, true, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL, NULL);
    }
  case CUBEC_VALUE_TYPE_INT16:
    if (data) {
      return create_i16(ctx, *data, true, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL, NULL);
    }
  case CUBEC_VALUE_TYPE_INT32:
    if (data) {
      return create_i32(ctx, *data, true, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL, NULL);
    }
  case CUBEC_VALUE_TYPE_INT64:
    if (data) {
      return create_i64(ctx, *data, true, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL, NULL);
    }
  case CUBEC_VALUE_TYPE_UINT8:
    if (data) {
      return create_u8(ctx, *data, true, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL, NULL);
    }
  case CUBEC_VALUE_TYPE_UINT16:
    if (data) {
      return create_u16(ctx, *data, true, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL, NULL);
    }
  case CUBEC_VALUE_TYPE_UINT32:
    if (data) {
      return create_u32(ctx, *data, true, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL, NULL);
    }
  case CUBEC_VALUE_TYPE_UINT64:
    if (data) {
      return create_u64(ctx, *data, true, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL, NULL);
    }
  case CUBEC_VALUE_TYPE_FLOAT32:
    if (data) {
      return create_f32(ctx, *data, true, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL, NULL);
    }
  case CUBEC_VALUE_TYPE_FLOAT64:
    if (data) {
      return create_f64(ctx, *data, true, NULL);
    } else {
      return context_create_value(ctx, type, false, NULL, NULL);
    }
  default:
    break;
  }
  return NULL;
}

void init_boolean_type(context_t ctx) {
  struct _type_operator_t opt = {
      .type_to_string = boolean_type_to_string,
      .to_string = boolean_to_string,
      .convert = boolean_convert,
      .logical_and_opt = boolean_logical_and,
      .logical_or_opt = boolean_logical_or,
      .logical_not_opt = boolean_logical_not,
  };
  context_create_type(ctx, CUBEC_VALUE_TYPE_BOOL, sizeof(bool), sizeof(bool),
                      NULL, &opt, "bool");
}
value_t create_boolean(context_t ctx, bool value, bool mutable,
                       const char *name) {
  value_t vtype = context_load(ctx, "bool");
  type_t type = *(type_t *)value_get_data(vtype);
  return context_create_value(ctx, type, mutable, &value, name);
}