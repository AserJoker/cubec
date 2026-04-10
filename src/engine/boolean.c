#include "engine/boolean.h"
#include "core/position.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/numeric.h"
#include "engine/str.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdbool.h>

static char *cubec_boolean_type_to_string(cubec_type_t self,
                                          cubec_allocator_t allocator) {
  return cubec_create_cstring(allocator, "bool");
}

static cubec_value_t cubec_boolean_to_string(cubec_value_t value,
                                             cubec_context_t ctx) {
  bool val = *(bool *)cubec_value_get_data(value);
  const char *str = val ? "true" : "false";
  return cubec_create_str(ctx, str, NULL);
}

static cubec_value_t cubec_boolean_convert(cubec_value_t value,
                                           cubec_context_t ctx,
                                           cubec_type_t type) {
  bool *data = cubec_value_get_data(value);
  cubec_type_kind_t kind = cubec_type_get_kind(type);
  switch (kind) {
  case CUBEC_VALUE_TYPE_BOOL:
    if (data) {
      return cubec_create_boolean(ctx, *data, true, NULL);
    } else {
      return cubec_context_create_value(ctx, type, false, NULL, NULL);
    }
  case CUBEC_VALUE_TYPE_INT8:
    if (data) {
      return cubec_create_i8(ctx, *data, true, NULL);
    } else {
      return cubec_context_create_value(ctx, type, false, NULL, NULL);
    }
  case CUBEC_VALUE_TYPE_INT16:
    if (data) {
      return cubec_create_i16(ctx, *data, true, NULL);
    } else {
      return cubec_context_create_value(ctx, type, false, NULL, NULL);
    }
  case CUBEC_VALUE_TYPE_INT32:
    if (data) {
      return cubec_create_i32(ctx, *data, true, NULL);
    } else {
      return cubec_context_create_value(ctx, type, false, NULL, NULL);
    }
  case CUBEC_VALUE_TYPE_INT64:
    if (data) {
      return cubec_create_i64(ctx, *data, true, NULL);
    } else {
      return cubec_context_create_value(ctx, type, false, NULL, NULL);
    }
  case CUBEC_VALUE_TYPE_UINT8:
    if (data) {
      return cubec_create_u8(ctx, *data, true, NULL);
    } else {
      return cubec_context_create_value(ctx, type, false, NULL, NULL);
    }
  case CUBEC_VALUE_TYPE_UINT16:
    if (data) {
      return cubec_create_u16(ctx, *data, true, NULL);
    } else {
      return cubec_context_create_value(ctx, type, false, NULL, NULL);
    }
  case CUBEC_VALUE_TYPE_UINT32:
    if (data) {
      return cubec_create_u32(ctx, *data, true, NULL);
    } else {
      return cubec_context_create_value(ctx, type, false, NULL, NULL);
    }
  case CUBEC_VALUE_TYPE_UINT64:
    if (data) {
      return cubec_create_u64(ctx, *data, true, NULL);
    } else {
      return cubec_context_create_value(ctx, type, false, NULL, NULL);
    }
  case CUBEC_VALUE_TYPE_FLOAT32:
    if (data) {
      return cubec_create_f32(ctx, *data, true, NULL);
    } else {
      return cubec_context_create_value(ctx, type, false, NULL, NULL);
    }
  case CUBEC_VALUE_TYPE_FLOAT64:
    if (data) {
      return cubec_create_f64(ctx, *data, true, NULL);
    } else {
      return cubec_context_create_value(ctx, type, false, NULL, NULL);
    }
  default:
    break;
  }
  return NULL;
}

void cubec_init_boolean_type(cubec_context_t ctx) {
  struct _cubec_type_operator_t opt = {
      .type_to_string = cubec_boolean_type_to_string,
      .to_string = cubec_boolean_to_string,
      .convert = cubec_boolean_convert,
  };
  cubec_context_create_type(ctx, CUBEC_VALUE_TYPE_BOOL, sizeof(bool),
                            sizeof(bool), NULL, &opt, "bool");
}
cubec_value_t cubec_create_boolean(cubec_context_t ctx, bool value,
                                   bool mutable, const char *name) {
  cubec_value_t vtype = cubec_context_load(ctx, "bool");
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  return cubec_context_create_value(ctx, type, mutable, &value, name);
}