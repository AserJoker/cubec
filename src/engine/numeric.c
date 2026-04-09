#include "engine/numeric.h"
#include "core/allocator.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/error.h"
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
  cubec_context_create_type(ctx, CUBEC_VALUE_TYPE_FLOAT##size,                 \
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
  case CUBEC_VALUE_TYPE_FLOAT32:
    sprintf(str, "%g", *(float32_t *)data);
    break;
  case CUBEC_VALUE_TYPE_FLOAT64:
    sprintf(str, "%g", *(float64_t *)data);
    break;
  default:
    break;
  }
  return cubec_create_str(ctx, str, NULL);
}

#define DECLAR_BINARY_OPT(type, name, opt)                                     \
  static cubec_value_t cubec_##type##_##name(                                  \
      cubec_value_t self, cubec_context_t ctx, cubec_value_t another) {        \
    type##_t left = *(type##_t *)cubec_value_get_data(self);                   \
    type##_t right = *(type##_t *)cubec_value_get_data(another);               \
    return cubec_create_##type(ctx, left opt right, true, NULL);               \
  }
DECLAR_BINARY_OPT(int8, add, +);
DECLAR_BINARY_OPT(int8, sub, -);
DECLAR_BINARY_OPT(int8, div, /);
DECLAR_BINARY_OPT(int8, mul, *);
DECLAR_BINARY_OPT(int8, mod, %);
DECLAR_BINARY_OPT(int8, and, &);
DECLAR_BINARY_OPT(int8, or, |);
DECLAR_BINARY_OPT(int8, xor, ^);
DECLAR_BINARY_OPT(int8, shl, <<);
DECLAR_BINARY_OPT(int8, shr, >>);
DECLAR_BINARY_OPT(int16, add, +);
DECLAR_BINARY_OPT(int16, sub, -);
DECLAR_BINARY_OPT(int16, div, /);
DECLAR_BINARY_OPT(int16, mul, *);
DECLAR_BINARY_OPT(int16, mod, %);
DECLAR_BINARY_OPT(int16, and, &);
DECLAR_BINARY_OPT(int16, or, |);
DECLAR_BINARY_OPT(int16, xor, ^);
DECLAR_BINARY_OPT(int16, shl, <<);
DECLAR_BINARY_OPT(int16, shr, >>);
DECLAR_BINARY_OPT(int32, add, +);
DECLAR_BINARY_OPT(int32, sub, -);
DECLAR_BINARY_OPT(int32, div, /);
DECLAR_BINARY_OPT(int32, mul, *);
DECLAR_BINARY_OPT(int32, mod, %);
DECLAR_BINARY_OPT(int32, and, &);
DECLAR_BINARY_OPT(int32, or, |);
DECLAR_BINARY_OPT(int32, xor, ^);
DECLAR_BINARY_OPT(int32, shl, <<);
DECLAR_BINARY_OPT(int32, shr, >>);
DECLAR_BINARY_OPT(int64, add, +);
DECLAR_BINARY_OPT(int64, sub, -);
DECLAR_BINARY_OPT(int64, div, /);
DECLAR_BINARY_OPT(int64, mul, *);
DECLAR_BINARY_OPT(int64, mod, %);
DECLAR_BINARY_OPT(int64, and, &);
DECLAR_BINARY_OPT(int64, or, |);
DECLAR_BINARY_OPT(int64, xor, ^);
DECLAR_BINARY_OPT(int64, shl, <<);
DECLAR_BINARY_OPT(int64, shr, >>);

DECLAR_BINARY_OPT(uint8, add, +);
DECLAR_BINARY_OPT(uint8, sub, -);
DECLAR_BINARY_OPT(uint8, div, /);
DECLAR_BINARY_OPT(uint8, mul, *);
DECLAR_BINARY_OPT(uint8, mod, %);
DECLAR_BINARY_OPT(uint8, and, &);
DECLAR_BINARY_OPT(uint8, or, |);
DECLAR_BINARY_OPT(uint8, xor, ^);
DECLAR_BINARY_OPT(uint8, shl, <<);
DECLAR_BINARY_OPT(uint8, shr, >>);
DECLAR_BINARY_OPT(uint16, add, +);
DECLAR_BINARY_OPT(uint16, sub, -);
DECLAR_BINARY_OPT(uint16, div, /);
DECLAR_BINARY_OPT(uint16, mul, *);
DECLAR_BINARY_OPT(uint16, mod, %);
DECLAR_BINARY_OPT(uint16, and, &);
DECLAR_BINARY_OPT(uint16, or, |);
DECLAR_BINARY_OPT(uint16, xor, ^);
DECLAR_BINARY_OPT(uint16, shl, <<);
DECLAR_BINARY_OPT(uint16, shr, >>);
DECLAR_BINARY_OPT(uint32, add, +);
DECLAR_BINARY_OPT(uint32, sub, -);
DECLAR_BINARY_OPT(uint32, div, /);
DECLAR_BINARY_OPT(uint32, mul, *);
DECLAR_BINARY_OPT(uint32, mod, %);
DECLAR_BINARY_OPT(uint32, and, &);
DECLAR_BINARY_OPT(uint32, or, |);
DECLAR_BINARY_OPT(uint32, xor, ^);
DECLAR_BINARY_OPT(uint32, shl, <<);
DECLAR_BINARY_OPT(uint32, shr, >>);
DECLAR_BINARY_OPT(uint64, add, +);
DECLAR_BINARY_OPT(uint64, sub, -);
DECLAR_BINARY_OPT(uint64, div, /);
DECLAR_BINARY_OPT(uint64, mul, *);
DECLAR_BINARY_OPT(uint64, mod, %);
DECLAR_BINARY_OPT(uint64, and, &);
DECLAR_BINARY_OPT(uint64, or, |);
DECLAR_BINARY_OPT(uint64, xor, ^);
DECLAR_BINARY_OPT(uint64, shl, <<);
DECLAR_BINARY_OPT(uint64, shr, >>);

DECLAR_BINARY_OPT(float32, add, +);
DECLAR_BINARY_OPT(float32, sub, -);
DECLAR_BINARY_OPT(float32, div, /);
DECLAR_BINARY_OPT(float32, mul, *);

DECLAR_BINARY_OPT(float64, add, +);
DECLAR_BINARY_OPT(float64, sub, -);
DECLAR_BINARY_OPT(float64, div, /);
DECLAR_BINARY_OPT(float64, mul, *);

#define DECLAR_CONVERT(type)                                                   \
  static cubec_value_t cubec_##type##_convert(                                 \
      cubec_value_t self, cubec_context_t ctx, cubec_type_t type) {            \
    type##_t value = *(type##_t *)cubec_value_get_data(self);                  \
    cubec_type_kind_t kind = cubec_type_get_kind(type);                        \
    switch (kind) {                                                            \
    case CUBEC_VALUE_TYPE_INT8:                                                \
      return cubec_create_int8(ctx, value, false, NULL);                       \
    case CUBEC_VALUE_TYPE_INT16:                                               \
      return cubec_create_int16(ctx, value, false, NULL);                      \
    case CUBEC_VALUE_TYPE_INT32:                                               \
      return cubec_create_int32(ctx, value, false, NULL);                      \
    case CUBEC_VALUE_TYPE_INT64:                                               \
      return cubec_create_int64(ctx, value, false, NULL);                      \
    case CUBEC_VALUE_TYPE_UINT8:                                               \
      return cubec_create_uint8(ctx, value, false, NULL);                      \
    case CUBEC_VALUE_TYPE_UINT16:                                              \
      return cubec_create_uint16(ctx, value, false, NULL);                     \
    case CUBEC_VALUE_TYPE_UINT32:                                              \
      return cubec_create_uint32(ctx, value, false, NULL);                     \
    case CUBEC_VALUE_TYPE_UINT64:                                              \
      return cubec_create_uint64(ctx, value, false, NULL);                     \
    case CUBEC_VALUE_TYPE_FLOAT32:                                             \
      return cubec_create_float32(ctx, value, false, NULL);                    \
    case CUBEC_VALUE_TYPE_FLOAT64:                                             \
      return cubec_create_float64(ctx, value, false, NULL);                    \
    default:                                                                   \
      break;                                                                   \
    }                                                                          \
    cubec_allocator_t allocator = cubec_context_get_allocator(ctx);            \
    char *dst_name =                                                           \
        cubec_type_to_string(cubec_value_get_type(self), allocator);           \
    char *src_name = cubec_type_to_string(type, allocator);                    \
    cubec_value_t err = cubec_create_error(ctx, "Cannot convert '%s' to '%s'", \
                                           src_name, dst_name);                \
    cubec_allocator_free(allocator, src_name);                                 \
    cubec_allocator_free(allocator, dst_name);                                 \
    return err;                                                                \
  }

DECLAR_CONVERT(int8);
DECLAR_CONVERT(int16);
DECLAR_CONVERT(int32);
DECLAR_CONVERT(int64);
DECLAR_CONVERT(uint8);
DECLAR_CONVERT(uint16);
DECLAR_CONVERT(uint32);
DECLAR_CONVERT(uint64);
DECLAR_CONVERT(float32);
DECLAR_CONVERT(float64);
void cubec_init_numeric_type(cubec_context_t ctx) {
  struct _cubec_type_operator_t i8_opt = {
      .type_to_string = &cubec_numeric_type_to_string,
      .to_string = &cubec_numeric_to_string,
      .add_opt = cubec_int8_add,
      .sub_opt = cubec_int8_add,
      .div_opt = cubec_int8_add,
      .mul_opt = cubec_int8_add,
      .mod_opt = cubec_int8_add,
      .and_opt = cubec_int8_add,
      .or_opt = cubec_int8_add,
      .xor_opt = cubec_int8_add,
      .shl_opt = cubec_int8_add,
      .shr_opt = cubec_int8_add,
      .convert = cubec_int8_convert,
  };
  DECLAR_INTEGER(8, &i8_opt);
  struct _cubec_type_operator_t i16_opt = {
      .type_to_string = &cubec_numeric_type_to_string,
      .to_string = &cubec_numeric_to_string,
      .add_opt = cubec_int16_add,
      .sub_opt = cubec_int16_add,
      .div_opt = cubec_int16_add,
      .mul_opt = cubec_int16_add,
      .mod_opt = cubec_int16_add,
      .and_opt = cubec_int16_add,
      .or_opt = cubec_int16_add,
      .xor_opt = cubec_int16_add,
      .shl_opt = cubec_int16_add,
      .shr_opt = cubec_int16_add,
      .convert = cubec_int16_convert,
  };
  DECLAR_INTEGER(16, &i16_opt);
  struct _cubec_type_operator_t i32_opt = {
      .type_to_string = &cubec_numeric_type_to_string,
      .to_string = &cubec_numeric_to_string,
      .add_opt = cubec_int32_add,
      .sub_opt = cubec_int32_add,
      .div_opt = cubec_int32_add,
      .mul_opt = cubec_int32_add,
      .mod_opt = cubec_int32_add,
      .and_opt = cubec_int32_add,
      .or_opt = cubec_int32_add,
      .xor_opt = cubec_int32_add,
      .shl_opt = cubec_int32_add,
      .shr_opt = cubec_int32_add,
      .convert = cubec_int32_convert,
  };
  DECLAR_INTEGER(32, &i32_opt);
  struct _cubec_type_operator_t i64_opt = {
      .type_to_string = &cubec_numeric_type_to_string,
      .to_string = &cubec_numeric_to_string,
      .add_opt = cubec_int64_add,
      .sub_opt = cubec_int64_add,
      .div_opt = cubec_int64_add,
      .mul_opt = cubec_int64_add,
      .mod_opt = cubec_int64_add,
      .and_opt = cubec_int64_add,
      .or_opt = cubec_int64_add,
      .xor_opt = cubec_int64_add,
      .shl_opt = cubec_int64_add,
      .shr_opt = cubec_int64_add,
      .convert = cubec_int64_convert,
  };
  DECLAR_INTEGER(64, &i64_opt);
  struct _cubec_type_operator_t u8_opt = {
      .type_to_string = &cubec_numeric_type_to_string,
      .to_string = &cubec_numeric_to_string,
      .add_opt = cubec_uint8_add,
      .sub_opt = cubec_uint8_add,
      .div_opt = cubec_uint8_add,
      .mul_opt = cubec_uint8_add,
      .mod_opt = cubec_uint8_add,
      .and_opt = cubec_uint8_add,
      .or_opt = cubec_uint8_add,
      .xor_opt = cubec_uint8_add,
      .shl_opt = cubec_uint8_add,
      .shr_opt = cubec_uint8_add,
      .convert = cubec_uint8_convert,
  };
  DECLAR_UNSIGNED(8, &u8_opt);
  struct _cubec_type_operator_t u16_opt = {
      .type_to_string = &cubec_numeric_type_to_string,
      .to_string = &cubec_numeric_to_string,
      .add_opt = cubec_uint16_add,
      .sub_opt = cubec_uint16_add,
      .div_opt = cubec_uint16_add,
      .mul_opt = cubec_uint16_add,
      .mod_opt = cubec_uint16_add,
      .and_opt = cubec_uint16_add,
      .or_opt = cubec_uint16_add,
      .xor_opt = cubec_uint16_add,
      .shl_opt = cubec_uint16_add,
      .shr_opt = cubec_uint16_add,
      .convert = cubec_uint16_convert,
  };
  DECLAR_UNSIGNED(16, &u16_opt);
  struct _cubec_type_operator_t u32_opt = {
      .type_to_string = &cubec_numeric_type_to_string,
      .to_string = &cubec_numeric_to_string,
      .add_opt = cubec_uint32_add,
      .sub_opt = cubec_uint32_add,
      .div_opt = cubec_uint32_add,
      .mul_opt = cubec_uint32_add,
      .mod_opt = cubec_uint32_add,
      .and_opt = cubec_uint32_add,
      .or_opt = cubec_uint32_add,
      .xor_opt = cubec_uint32_add,
      .shl_opt = cubec_uint32_add,
      .shr_opt = cubec_uint32_add,
      .convert = cubec_uint32_convert,
  };
  DECLAR_UNSIGNED(32, &u32_opt);
  struct _cubec_type_operator_t u64_opt = {
      .type_to_string = &cubec_numeric_type_to_string,
      .to_string = &cubec_numeric_to_string,
      .add_opt = cubec_uint64_add,
      .sub_opt = cubec_uint64_add,
      .div_opt = cubec_uint64_add,
      .mul_opt = cubec_uint64_add,
      .mod_opt = cubec_uint64_add,
      .and_opt = cubec_uint64_add,
      .or_opt = cubec_uint64_add,
      .xor_opt = cubec_uint64_add,
      .shl_opt = cubec_uint64_add,
      .shr_opt = cubec_uint64_add,
      .convert = cubec_uint64_convert,
  };
  DECLAR_UNSIGNED(64, &u64_opt);
  struct _cubec_type_operator_t f32_opt = {
      .type_to_string = &cubec_numeric_type_to_string,
      .to_string = &cubec_numeric_to_string,
      .add_opt = cubec_float32_add,
      .sub_opt = cubec_float32_sub,
      .mul_opt = cubec_float32_mul,
      .div_opt = cubec_float32_div,
      .convert = cubec_float32_convert,
  };
  DECLAR_FLOAT(32, &f32_opt);
  struct _cubec_type_operator_t f64_opt = {
      .type_to_string = &cubec_numeric_type_to_string,
      .to_string = &cubec_numeric_to_string,
      .add_opt = cubec_float64_add,
      .sub_opt = cubec_float64_sub,
      .mul_opt = cubec_float64_mul,
      .div_opt = cubec_float64_div,
      .convert = cubec_float64_convert,
  };
  DECLAR_FLOAT(64, &f64_opt);
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
