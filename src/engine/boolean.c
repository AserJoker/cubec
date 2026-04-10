#include "engine/boolean.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/numeric.h"
#include "engine/str.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdbool.h>
#include <stdlib.h>

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
  bool val = *(bool *)cubec_value_get_data(value);
  cubec_type_kind_t kind = cubec_type_get_kind(type);
  switch (kind) {
  case CUBEC_VALUE_TYPE_BOOL:
    return cubec_create_boolean(ctx, val, true, NULL);
  case CUBEC_VALUE_TYPE_INT8:
    return cubec_create_int8(ctx, val, true, NULL);
  case CUBEC_VALUE_TYPE_INT16:
    return cubec_create_int16(ctx, val, true, NULL);
  case CUBEC_VALUE_TYPE_INT32:
    return cubec_create_int32(ctx, val, true, NULL);
  case CUBEC_VALUE_TYPE_INT64:
    return cubec_create_int64(ctx, val, true, NULL);
  case CUBEC_VALUE_TYPE_UINT8:
    return cubec_create_uint8(ctx, val, true, NULL);
  case CUBEC_VALUE_TYPE_UINT16:
    return cubec_create_uint16(ctx, val, true, NULL);
  case CUBEC_VALUE_TYPE_UINT32:
    return cubec_create_uint32(ctx, val, true, NULL);
  case CUBEC_VALUE_TYPE_UINT64:
    return cubec_create_uint64(ctx, val, true, NULL);
  case CUBEC_VALUE_TYPE_FLOAT32:
    return cubec_create_float32(ctx, val, true, NULL);
  case CUBEC_VALUE_TYPE_FLOAT64:
    return cubec_create_float64(ctx, val, true, NULL);
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