#include "engine/numeric.h"
#include "core/allocator.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/str.h"
#include "engine/type.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#define DECLAR_INTEGER(size, opt)                                              \
  cubec_context_create_type(ctx, CUBEC_VALUE_TYPE_INT##size,                   \
                            sizeof(int##size##_t), sizeof(int##size##_t),      \
                            NULL, opt, "i" #size)
#define DECLAR_UNSIGNED(size, opt)                                             \
  cubec_context_create_type(ctx, CUBEC_VALUE_TYPE_UINT##size,                  \
                            sizeof(uint##size##_t), sizeof(uint##size##_t),    \
                            NULL, opt, "u" #size)
#define DECLAR_FLOAT(size, opt)                                                \
  cubec_context_create_type(ctx, CUBEC_VALUE_TYPE_UINT##size,                  \
                            sizeof(float##size##_t), sizeof(float##size##_t),  \
                            NULL, opt, "f" #size)

static char *cubec_numeric_type_to_string(cubec_type_t self,
                                          cubec_allocator_t allocator) {
  cubec_type_kind_t kind = cubec_type_get_kind(self);
  switch (kind) {
  case CUBEC_VALUE_TYPE_INT8:
    return cubec_create_cstring(allocator, "i8");
  case CUBEC_VALUE_TYPE_INT16:
    return cubec_create_cstring(allocator, "i16");
  case CUBEC_VALUE_TYPE_INT32:
    return cubec_create_cstring(allocator, "i32");
  case CUBEC_VALUE_TYPE_INT64:
    return cubec_create_cstring(allocator, "i64");
  case CUBEC_VALUE_TYPE_UINT8:
    return cubec_create_cstring(allocator, "u8");
  case CUBEC_VALUE_TYPE_UINT16:
    return cubec_create_cstring(allocator, "u16");
  case CUBEC_VALUE_TYPE_UINT32:
    return cubec_create_cstring(allocator, "u32");
  case CUBEC_VALUE_TYPE_UINT64:
    return cubec_create_cstring(allocator, "u64");
  case CUBEC_VALUE_TYPE_FLOAT16:
    return cubec_create_cstring(allocator, "f16");
  case CUBEC_VALUE_TYPE_FLOAT32:
    return cubec_create_cstring(allocator, "f32");
  case CUBEC_VALUE_TYPE_FLOAT64:
    return cubec_create_cstring(allocator, "f64");
  default:
    break;
  }
  return NULL;
}
static cubec_value_t cubec_numeric_to_string(cubec_value_t self,
                                             cubec_context_t ctx) {
  cubec_type_t type = cubec_value_get_type(self);
  cubec_type_kind_t kind = cubec_type_get_kind(type);
  char str[32] = {0};
  void *data = cubec_value_get_data(self);
  switch (kind) {
  case CUBEC_VALUE_TYPE_INT8:
    sprintf(str, "%d", *(int8_t *)data);
    break;
  case CUBEC_VALUE_TYPE_INT16:
    sprintf(str, "%d", *(int16_t *)data);
    break;
  case CUBEC_VALUE_TYPE_INT32:
    sprintf(str, "%d", *(int32_t *)data);
    break;
  case CUBEC_VALUE_TYPE_INT64:
    sprintf(str, "%" PRIdPTR, *(int64_t *)data);
    break;
  case CUBEC_VALUE_TYPE_UINT8:
    sprintf(str, "%u", *(uint8_t *)data);
    break;
  case CUBEC_VALUE_TYPE_UINT16:
    sprintf(str, "%u", *(uint16_t *)data);
    break;
  case CUBEC_VALUE_TYPE_UINT32:
    sprintf(str, "%u", *(uint32_t *)data);
    break;
  case CUBEC_VALUE_TYPE_UINT64:
    sprintf(str, "%" PRIuPTR, *(uint64_t *)data);
    break;
  case CUBEC_VALUE_TYPE_FLOAT16:
    sprintf(str, "%g", (double)*(float16_t *)data);
  case CUBEC_VALUE_TYPE_FLOAT32:
    sprintf(str, "%g", *(float32_t *)data);
  case CUBEC_VALUE_TYPE_FLOAT64:
    sprintf(str, "%g", *(float64_t *)data);
  default:
    break;
  }
  return cubec_create_str(ctx, str, NULL);
}

void cubec_init_numeric_type(cubec_context_t ctx) {
  struct _cubec_type_operator_t opt = {
      .type_to_string = &cubec_numeric_type_to_string,
      .to_string = &cubec_numeric_to_string,
  };
  DECLAR_INTEGER(8, &opt);
  DECLAR_INTEGER(16, &opt);
  DECLAR_INTEGER(32, &opt);
  DECLAR_INTEGER(64, &opt);
  DECLAR_UNSIGNED(8, &opt);
  DECLAR_UNSIGNED(16, &opt);
  DECLAR_UNSIGNED(32, &opt);
  DECLAR_UNSIGNED(64, &opt);
  DECLAR_FLOAT(16, &opt);
  DECLAR_FLOAT(32, &opt);
  DECLAR_FLOAT(64, &opt);
}
cubec_value_t cubec_create_int8(cubec_context_t ctx, int8_t value, bool mutable,
                                const char *name) {
  cubec_value_t vtype = cubec_context_load(ctx, "i8");
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  return cubec_context_create_value(ctx, type, mutable, &value, name);
}
cubec_value_t cubec_create_int16(cubec_context_t ctx, int16_t value,
                                 bool mutable, const char *name) {
  cubec_value_t vtype = cubec_context_load(ctx, "i16");
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  return cubec_context_create_value(ctx, type, mutable, &value, name);
}
cubec_value_t cubec_create_int32(cubec_context_t ctx, int32_t value,
                                 bool mutable, const char *name) {
  cubec_value_t vtype = cubec_context_load(ctx, "i32");
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  return cubec_context_create_value(ctx, type, mutable, &value, name);
}
cubec_value_t cubec_create_int64(cubec_context_t ctx, int64_t value,
                                 bool mutable, const char *name) {
  cubec_value_t vtype = cubec_context_load(ctx, "i64");
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  return cubec_context_create_value(ctx, type, mutable, &value, name);
}
cubec_value_t cubec_create_uint8(cubec_context_t ctx, uint8_t value,
                                 bool mutable, const char *name) {
  cubec_value_t vtype = cubec_context_load(ctx, "u8");
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  return cubec_context_create_value(ctx, type, mutable, &value, name);
}
cubec_value_t cubec_create_uint16(cubec_context_t ctx, uint16_t value,
                                  bool mutable, const char *name) {
  cubec_value_t vtype = cubec_context_load(ctx, "u16");
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  return cubec_context_create_value(ctx, type, mutable, &value, name);
}
cubec_value_t cubec_create_uint32(cubec_context_t ctx, uint32_t value,
                                  bool mutable, const char *name) {
  cubec_value_t vtype = cubec_context_load(ctx, "u32");
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  return cubec_context_create_value(ctx, type, mutable, &value, name);
}
cubec_value_t cubec_create_uint64(cubec_context_t ctx, uint64_t value,
                                  bool mutable, const char *name) {
  cubec_value_t vtype = cubec_context_load(ctx, "u64");
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  return cubec_context_create_value(ctx, type, mutable, &value, name);
}
cubec_value_t cubec_create_float16(cubec_context_t ctx, float16_t value,
                                   bool mutable, const char *name) {
  cubec_value_t vtype = cubec_context_load(ctx, "f16");
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  return cubec_context_create_value(ctx, type, mutable, &value, name);
}
cubec_value_t cubec_create_float32(cubec_context_t ctx, float32_t value,
                                   bool mutable, const char *name) {
  cubec_value_t vtype = cubec_context_load(ctx, "f32");
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  return cubec_context_create_value(ctx, type, mutable, &value, name);
}
cubec_value_t cubec_create_float64(cubec_context_t ctx, float64_t value,
                                   bool mutable, const char *name) {
  cubec_value_t vtype = cubec_context_load(ctx, "f64");
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  return cubec_context_create_value(ctx, type, mutable, &value, name);
}