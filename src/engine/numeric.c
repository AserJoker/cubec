#include "engine/numeric.h"
#include "core/allocator.h"
#include "core/string.h"
#include "engine/boolean.h"
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
                            sizeof(i##size##_t), sizeof(i##size##_t), NULL,    \
                            opt, "i" #size)
#define DECLAR_UNSIGNED(size, opt)                                             \
  cubec_context_create_type(ctx, CUBEC_VALUE_TYPE_UINT##size,                  \
                            sizeof(u##size##_t), sizeof(u##size##_t), NULL,    \
                            opt, "u" #size)
#define DECLAR_FLOAT(size, opt)                                                \
  cubec_context_create_type(ctx, CUBEC_VALUE_TYPE_FLOAT##size,                 \
                            sizeof(f##size##_t), sizeof(f##size##_t), NULL,    \
                            opt, "f" #size)

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
    sprintf(str, "%d", *(i8_t *)data);
    break;
  case CUBEC_VALUE_TYPE_INT16:
    sprintf(str, "%d", *(i16_t *)data);
    break;
  case CUBEC_VALUE_TYPE_INT32:
    sprintf(str, "%d", *(i32_t *)data);
    break;
  case CUBEC_VALUE_TYPE_INT64:
    sprintf(str, "%" PRIdPTR, *(i64_t *)data);
    break;
  case CUBEC_VALUE_TYPE_UINT8:
    sprintf(str, "%u", *(u8_t *)data);
    break;
  case CUBEC_VALUE_TYPE_UINT16:
    sprintf(str, "%u", *(u16_t *)data);
    break;
  case CUBEC_VALUE_TYPE_UINT32:
    sprintf(str, "%u", *(u32_t *)data);
    break;
  case CUBEC_VALUE_TYPE_UINT64:
    sprintf(str, "%" PRIuPTR, *(u64_t *)data);
    break;
  case CUBEC_VALUE_TYPE_FLOAT32:
    sprintf(str, "%g", *(f32_t *)data);
    break;
  case CUBEC_VALUE_TYPE_FLOAT64:
    sprintf(str, "%g", *(f64_t *)data);
    break;
  default:
    break;
  }
  return cubec_create_str(ctx, str, NULL);
}

#define DECLAR_BINARY_OPT(type, rtype, name, opt)                              \
  static cubec_value_t cubec_##type##_##name(                                  \
      cubec_value_t self, cubec_context_t ctx, cubec_value_t another) {        \
    if (!cubec_value_get_data(self) || !cubec_value_get_data(another)) {       \
      cubec_value_t vtype = cubec_context_load(ctx, #rtype);                   \
      cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);        \
      return cubec_context_create_value(ctx, type, false, NULL, NULL);         \
    }                                                                          \
    type##_t left = *(type##_t *)cubec_value_get_data(self);                   \
    type##_t right = *(type##_t *)cubec_value_get_data(another);               \
    return cubec_create_##rtype(ctx, left opt right, true, NULL);              \
  }
DECLAR_BINARY_OPT(i8, i8, add, +);
DECLAR_BINARY_OPT(i8, i8, sub, -);
DECLAR_BINARY_OPT(i8, i8, div, /);
DECLAR_BINARY_OPT(i8, i8, mul, *);
DECLAR_BINARY_OPT(i8, i8, mod, %);
DECLAR_BINARY_OPT(i8, i8, and, &);
DECLAR_BINARY_OPT(i8, i8, or, |);
DECLAR_BINARY_OPT(i8, i8, xor, ^);
DECLAR_BINARY_OPT(i8, i8, shl, <<);
DECLAR_BINARY_OPT(i8, i8, shr, >>);

DECLAR_BINARY_OPT(i8, boolean, eq, ==);
DECLAR_BINARY_OPT(i8, boolean, ne, !=);
DECLAR_BINARY_OPT(i8, boolean, gt, >);
DECLAR_BINARY_OPT(i8, boolean, lt, <);
DECLAR_BINARY_OPT(i8, boolean, ge, >=);
DECLAR_BINARY_OPT(i8, boolean, le, <=);

DECLAR_BINARY_OPT(i16, i16, add, +);
DECLAR_BINARY_OPT(i16, i16, sub, -);
DECLAR_BINARY_OPT(i16, i16, div, /);
DECLAR_BINARY_OPT(i16, i16, mul, *);
DECLAR_BINARY_OPT(i16, i16, mod, %);
DECLAR_BINARY_OPT(i16, i16, and, &);
DECLAR_BINARY_OPT(i16, i16, or, |);
DECLAR_BINARY_OPT(i16, i16, xor, ^);
DECLAR_BINARY_OPT(i16, i16, shl, <<);
DECLAR_BINARY_OPT(i16, i16, shr, >>);

DECLAR_BINARY_OPT(i16, boolean, eq, ==);
DECLAR_BINARY_OPT(i16, boolean, ne, !=);
DECLAR_BINARY_OPT(i16, boolean, gt, >);
DECLAR_BINARY_OPT(i16, boolean, lt, <);
DECLAR_BINARY_OPT(i16, boolean, ge, >=);
DECLAR_BINARY_OPT(i16, boolean, le, <=);

DECLAR_BINARY_OPT(i32, i32, add, +);
DECLAR_BINARY_OPT(i32, i32, sub, -);
DECLAR_BINARY_OPT(i32, i32, div, /);
DECLAR_BINARY_OPT(i32, i32, mul, *);
DECLAR_BINARY_OPT(i32, i32, mod, %);
DECLAR_BINARY_OPT(i32, i32, and, &);
DECLAR_BINARY_OPT(i32, i32, or, |);
DECLAR_BINARY_OPT(i32, i32, xor, ^);
DECLAR_BINARY_OPT(i32, i32, shl, <<);
DECLAR_BINARY_OPT(i32, i32, shr, >>);

DECLAR_BINARY_OPT(i32, boolean, eq, ==);
DECLAR_BINARY_OPT(i32, boolean, ne, !=);
DECLAR_BINARY_OPT(i32, boolean, gt, >);
DECLAR_BINARY_OPT(i32, boolean, lt, <);
DECLAR_BINARY_OPT(i32, boolean, ge, >=);
DECLAR_BINARY_OPT(i32, boolean, le, <=);

DECLAR_BINARY_OPT(i64, i64, add, +);
DECLAR_BINARY_OPT(i64, i64, sub, -);
DECLAR_BINARY_OPT(i64, i64, div, /);
DECLAR_BINARY_OPT(i64, i64, mul, *);
DECLAR_BINARY_OPT(i64, i64, mod, %);
DECLAR_BINARY_OPT(i64, i64, and, &);
DECLAR_BINARY_OPT(i64, i64, or, |);
DECLAR_BINARY_OPT(i64, i64, xor, ^);
DECLAR_BINARY_OPT(i64, i64, shl, <<);
DECLAR_BINARY_OPT(i64, i64, shr, >>);

DECLAR_BINARY_OPT(i64, boolean, eq, ==);
DECLAR_BINARY_OPT(i64, boolean, ne, !=);
DECLAR_BINARY_OPT(i64, boolean, gt, >);
DECLAR_BINARY_OPT(i64, boolean, lt, <);
DECLAR_BINARY_OPT(i64, boolean, ge, >=);
DECLAR_BINARY_OPT(i64, boolean, le, <=);

DECLAR_BINARY_OPT(u8, u8, add, +);
DECLAR_BINARY_OPT(u8, u8, sub, -);
DECLAR_BINARY_OPT(u8, u8, div, /);
DECLAR_BINARY_OPT(u8, u8, mul, *);
DECLAR_BINARY_OPT(u8, u8, mod, %);
DECLAR_BINARY_OPT(u8, u8, and, &);
DECLAR_BINARY_OPT(u8, u8, or, |);
DECLAR_BINARY_OPT(u8, u8, xor, ^);
DECLAR_BINARY_OPT(u8, u8, shl, <<);
DECLAR_BINARY_OPT(u8, u8, shr, >>);

DECLAR_BINARY_OPT(u8, boolean, eq, ==);
DECLAR_BINARY_OPT(u8, boolean, ne, !=);
DECLAR_BINARY_OPT(u8, boolean, gt, >);
DECLAR_BINARY_OPT(u8, boolean, lt, <);
DECLAR_BINARY_OPT(u8, boolean, ge, >=);
DECLAR_BINARY_OPT(u8, boolean, le, <=);

DECLAR_BINARY_OPT(u16, u16, add, +);
DECLAR_BINARY_OPT(u16, u16, sub, -);
DECLAR_BINARY_OPT(u16, u16, div, /);
DECLAR_BINARY_OPT(u16, u16, mul, *);
DECLAR_BINARY_OPT(u16, u16, mod, %);
DECLAR_BINARY_OPT(u16, u16, and, &);
DECLAR_BINARY_OPT(u16, u16, or, |);
DECLAR_BINARY_OPT(u16, u16, xor, ^);
DECLAR_BINARY_OPT(u16, u16, shl, <<);
DECLAR_BINARY_OPT(u16, u16, shr, >>);

DECLAR_BINARY_OPT(u16, boolean, eq, ==);
DECLAR_BINARY_OPT(u16, boolean, ne, !=);
DECLAR_BINARY_OPT(u16, boolean, gt, >);
DECLAR_BINARY_OPT(u16, boolean, lt, <);
DECLAR_BINARY_OPT(u16, boolean, ge, >=);
DECLAR_BINARY_OPT(u16, boolean, le, <=);

DECLAR_BINARY_OPT(u32, u32, add, +);
DECLAR_BINARY_OPT(u32, u32, sub, -);
DECLAR_BINARY_OPT(u32, u32, div, /);
DECLAR_BINARY_OPT(u32, u32, mul, *);
DECLAR_BINARY_OPT(u32, u32, mod, %);
DECLAR_BINARY_OPT(u32, u32, and, &);
DECLAR_BINARY_OPT(u32, u32, or, |);
DECLAR_BINARY_OPT(u32, u32, xor, ^);
DECLAR_BINARY_OPT(u32, u32, shl, <<);
DECLAR_BINARY_OPT(u32, u32, shr, >>);

DECLAR_BINARY_OPT(u32, boolean, eq, ==);
DECLAR_BINARY_OPT(u32, boolean, ne, !=);
DECLAR_BINARY_OPT(u32, boolean, gt, >);
DECLAR_BINARY_OPT(u32, boolean, lt, <);
DECLAR_BINARY_OPT(u32, boolean, ge, >=);
DECLAR_BINARY_OPT(u32, boolean, le, <=);

DECLAR_BINARY_OPT(u64, u64, add, +);
DECLAR_BINARY_OPT(u64, u64, sub, -);
DECLAR_BINARY_OPT(u64, u64, div, /);
DECLAR_BINARY_OPT(u64, u64, mul, *);
DECLAR_BINARY_OPT(u64, u64, mod, %);
DECLAR_BINARY_OPT(u64, u64, and, &);
DECLAR_BINARY_OPT(u64, u64, or, |);
DECLAR_BINARY_OPT(u64, u64, xor, ^);
DECLAR_BINARY_OPT(u64, u64, shl, <<);
DECLAR_BINARY_OPT(u64, u64, shr, >>);

DECLAR_BINARY_OPT(u64, boolean, eq, ==);
DECLAR_BINARY_OPT(u64, boolean, ne, !=);
DECLAR_BINARY_OPT(u64, boolean, gt, >);
DECLAR_BINARY_OPT(u64, boolean, lt, <);
DECLAR_BINARY_OPT(u64, boolean, ge, >=);
DECLAR_BINARY_OPT(u64, boolean, le, <=);

DECLAR_BINARY_OPT(f32, f32, add, +);
DECLAR_BINARY_OPT(f32, f32, sub, -);
DECLAR_BINARY_OPT(f32, f32, div, /);
DECLAR_BINARY_OPT(f32, f32, mul, *);

DECLAR_BINARY_OPT(f32, boolean, eq, ==);
DECLAR_BINARY_OPT(f32, boolean, ne, !=);
DECLAR_BINARY_OPT(f32, boolean, gt, >);
DECLAR_BINARY_OPT(f32, boolean, lt, <);
DECLAR_BINARY_OPT(f32, boolean, ge, >=);
DECLAR_BINARY_OPT(f32, boolean, le, <=);

DECLAR_BINARY_OPT(f64, f64, add, +);
DECLAR_BINARY_OPT(f64, f64, sub, -);
DECLAR_BINARY_OPT(f64, f64, div, /);
DECLAR_BINARY_OPT(f64, f64, mul, *);

DECLAR_BINARY_OPT(f64, boolean, eq, ==);
DECLAR_BINARY_OPT(f64, boolean, ne, !=);
DECLAR_BINARY_OPT(f64, boolean, gt, >);
DECLAR_BINARY_OPT(f64, boolean, lt, <);
DECLAR_BINARY_OPT(f64, boolean, ge, >=);
DECLAR_BINARY_OPT(f64, boolean, le, <=);

#define DECLAR_CONVERT(type)                                                   \
  static cubec_value_t cubec_##type##_convert(                                 \
      cubec_value_t self, cubec_context_t ctx, cubec_type_t type) {            \
    type##_t *value = (type##_t *)cubec_value_get_data(self);                  \
    cubec_type_kind_t kind = cubec_type_get_kind(type);                        \
    switch (kind) {                                                            \
    case CUBEC_VALUE_TYPE_BOOL:                                                \
      if (value) {                                                             \
        return cubec_create_boolean(ctx, *value, false, NULL);                 \
      } else {                                                                 \
        return cubec_context_create_value(ctx, type, false, NULL, NULL);       \
      }                                                                        \
    case CUBEC_VALUE_TYPE_INT8:                                                \
      if (value) {                                                             \
        return cubec_create_i8(ctx, *value, false, NULL);                      \
      } else {                                                                 \
        return cubec_context_create_value(ctx, type, false, NULL, NULL);       \
      }                                                                        \
    case CUBEC_VALUE_TYPE_INT16:                                               \
      if (value) {                                                             \
        return cubec_create_i16(ctx, *value, false, NULL);                     \
      } else {                                                                 \
        return cubec_context_create_value(ctx, type, false, NULL, NULL);       \
      }                                                                        \
    case CUBEC_VALUE_TYPE_INT32:                                               \
      if (value) {                                                             \
        return cubec_create_i32(ctx, *value, false, NULL);                     \
      } else {                                                                 \
        return cubec_context_create_value(ctx, type, false, NULL, NULL);       \
      }                                                                        \
    case CUBEC_VALUE_TYPE_INT64:                                               \
      if (value) {                                                             \
        return cubec_create_i64(ctx, *value, false, NULL);                     \
      } else {                                                                 \
        return cubec_context_create_value(ctx, type, false, NULL, NULL);       \
      }                                                                        \
    case CUBEC_VALUE_TYPE_UINT8:                                               \
      if (value) {                                                             \
        return cubec_create_u8(ctx, *value, false, NULL);                      \
      } else {                                                                 \
        return cubec_context_create_value(ctx, type, false, NULL, NULL);       \
      }                                                                        \
    case CUBEC_VALUE_TYPE_UINT16:                                              \
      if (value) {                                                             \
        return cubec_create_u16(ctx, *value, false, NULL);                     \
      } else {                                                                 \
        return cubec_context_create_value(ctx, type, false, NULL, NULL);       \
      }                                                                        \
    case CUBEC_VALUE_TYPE_UINT32:                                              \
      if (value) {                                                             \
        return cubec_create_u32(ctx, *value, false, NULL);                     \
      } else {                                                                 \
        return cubec_context_create_value(ctx, type, false, NULL, NULL);       \
      }                                                                        \
    case CUBEC_VALUE_TYPE_UINT64:                                              \
      if (value) {                                                             \
        return cubec_create_u64(ctx, *value, false, NULL);                     \
      } else {                                                                 \
        return cubec_context_create_value(ctx, type, false, NULL, NULL);       \
      }                                                                        \
    case CUBEC_VALUE_TYPE_FLOAT32:                                             \
      if (value) {                                                             \
        return cubec_create_f32(ctx, *value, false, NULL);                     \
      } else {                                                                 \
        return cubec_context_create_value(ctx, type, false, NULL, NULL);       \
      }                                                                        \
    case CUBEC_VALUE_TYPE_FLOAT64:                                             \
      if (value) {                                                             \
        return cubec_create_f64(ctx, *value, false, NULL);                     \
      } else {                                                                 \
        return cubec_context_create_value(ctx, type, false, NULL, NULL);       \
      }                                                                        \
    default:                                                                   \
      break;                                                                   \
    }                                                                          \
    return NULL;                                                               \
  }

DECLAR_CONVERT(i8);
DECLAR_CONVERT(i16);
DECLAR_CONVERT(i32);
DECLAR_CONVERT(i64);
DECLAR_CONVERT(u8);
DECLAR_CONVERT(u16);
DECLAR_CONVERT(u32);
DECLAR_CONVERT(u64);
DECLAR_CONVERT(f32);
DECLAR_CONVERT(f64);
void cubec_init_numeric_type(cubec_context_t ctx) {
  struct _cubec_type_operator_t i8_opt = {
      .type_to_string = &cubec_numeric_type_to_string,
      .to_string = &cubec_numeric_to_string,
      .add_opt = cubec_i8_add,
      .sub_opt = cubec_i8_sub,
      .div_opt = cubec_i8_div,
      .mul_opt = cubec_i8_mul,
      .mod_opt = cubec_i8_mod,
      .and_opt = cubec_i8_and,
      .or_opt = cubec_i8_or,
      .xor_opt = cubec_i8_xor,
      .shl_opt = cubec_i8_shl,
      .shr_opt = cubec_i8_shr,

      .eq_opt = cubec_i8_eq,
      .ne_opt = cubec_i8_ne,
      .gt_opt = cubec_i8_gt,
      .lt_opt = cubec_i8_lt,
      .ge_opt = cubec_i8_ge,
      .le_opt = cubec_i8_le,

      .convert = cubec_i8_convert,
  };
  DECLAR_INTEGER(8, &i8_opt);
  struct _cubec_type_operator_t i16_opt = {
      .type_to_string = &cubec_numeric_type_to_string,
      .to_string = &cubec_numeric_to_string,
      .add_opt = cubec_i16_add,
      .sub_opt = cubec_i16_sub,
      .div_opt = cubec_i16_div,
      .mul_opt = cubec_i16_mul,
      .mod_opt = cubec_i16_mod,
      .and_opt = cubec_i16_and,
      .or_opt = cubec_i16_or,
      .xor_opt = cubec_i16_xor,
      .shl_opt = cubec_i16_shl,
      .shr_opt = cubec_i16_shr,

      .eq_opt = cubec_i16_eq,
      .ne_opt = cubec_i16_ne,
      .gt_opt = cubec_i16_gt,
      .lt_opt = cubec_i16_lt,
      .ge_opt = cubec_i16_ge,
      .le_opt = cubec_i16_le,

      .convert = cubec_i16_convert,
  };
  DECLAR_INTEGER(16, &i16_opt);
  struct _cubec_type_operator_t i32_opt = {
      .type_to_string = &cubec_numeric_type_to_string,
      .to_string = &cubec_numeric_to_string,
      .add_opt = cubec_i32_add,
      .sub_opt = cubec_i32_sub,
      .div_opt = cubec_i32_div,
      .mul_opt = cubec_i32_mul,
      .mod_opt = cubec_i32_mod,
      .and_opt = cubec_i32_and,
      .or_opt = cubec_i32_or,
      .xor_opt = cubec_i32_xor,
      .shl_opt = cubec_i32_shl,
      .shr_opt = cubec_i32_shr,

      .eq_opt = cubec_i32_eq,
      .ne_opt = cubec_i32_ne,
      .gt_opt = cubec_i32_gt,
      .lt_opt = cubec_i32_lt,
      .ge_opt = cubec_i32_ge,
      .le_opt = cubec_i32_le,

      .convert = cubec_i32_convert,
  };
  DECLAR_INTEGER(32, &i32_opt);
  struct _cubec_type_operator_t i64_opt = {
      .type_to_string = &cubec_numeric_type_to_string,
      .to_string = &cubec_numeric_to_string,
      .add_opt = cubec_i64_add,
      .sub_opt = cubec_i64_sub,
      .div_opt = cubec_i64_div,
      .mul_opt = cubec_i64_mul,
      .mod_opt = cubec_i64_mod,
      .and_opt = cubec_i64_and,
      .or_opt = cubec_i64_or,
      .xor_opt = cubec_i64_xor,
      .shl_opt = cubec_i64_shl,
      .shr_opt = cubec_i64_shr,

      .eq_opt = cubec_i64_eq,
      .ne_opt = cubec_i64_ne,
      .gt_opt = cubec_i64_gt,
      .lt_opt = cubec_i64_lt,
      .ge_opt = cubec_i64_ge,
      .le_opt = cubec_i64_le,

      .convert = cubec_i64_convert,
  };
  DECLAR_INTEGER(64, &i64_opt);
  struct _cubec_type_operator_t u8_opt = {
      .type_to_string = &cubec_numeric_type_to_string,
      .to_string = &cubec_numeric_to_string,
      .add_opt = cubec_u8_add,
      .sub_opt = cubec_u8_sub,
      .div_opt = cubec_u8_div,
      .mul_opt = cubec_u8_mul,
      .mod_opt = cubec_u8_mod,
      .and_opt = cubec_u8_and,
      .or_opt = cubec_u8_or,
      .xor_opt = cubec_u8_xor,
      .shl_opt = cubec_u8_shl,
      .shr_opt = cubec_u8_shr,

      .eq_opt = cubec_u8_eq,
      .ne_opt = cubec_u8_ne,
      .gt_opt = cubec_u8_gt,
      .lt_opt = cubec_u8_lt,
      .ge_opt = cubec_u8_ge,
      .le_opt = cubec_u8_le,

      .convert = cubec_u8_convert,
  };
  DECLAR_UNSIGNED(8, &u8_opt);
  struct _cubec_type_operator_t u16_opt = {
      .type_to_string = &cubec_numeric_type_to_string,
      .to_string = &cubec_numeric_to_string,
      .add_opt = cubec_u16_add,
      .sub_opt = cubec_u16_sub,
      .div_opt = cubec_u16_div,
      .mul_opt = cubec_u16_mul,
      .mod_opt = cubec_u16_mod,
      .and_opt = cubec_u16_and,
      .or_opt = cubec_u16_or,
      .xor_opt = cubec_u16_xor,
      .shl_opt = cubec_u16_shl,
      .shr_opt = cubec_u16_shr,

      .eq_opt = cubec_u16_eq,
      .ne_opt = cubec_u16_ne,
      .gt_opt = cubec_u16_gt,
      .lt_opt = cubec_u16_lt,
      .ge_opt = cubec_u16_ge,
      .le_opt = cubec_u16_le,

      .convert = cubec_u16_convert,
  };
  DECLAR_UNSIGNED(16, &u16_opt);
  struct _cubec_type_operator_t u32_opt = {
      .type_to_string = &cubec_numeric_type_to_string,
      .to_string = &cubec_numeric_to_string,
      .add_opt = cubec_u32_add,
      .sub_opt = cubec_u32_sub,
      .div_opt = cubec_u32_div,
      .mul_opt = cubec_u32_mul,
      .mod_opt = cubec_u32_mod,
      .and_opt = cubec_u32_and,
      .or_opt = cubec_u32_or,
      .xor_opt = cubec_u32_xor,
      .shl_opt = cubec_u32_shl,
      .shr_opt = cubec_u32_shr,

      .eq_opt = cubec_u32_eq,
      .ne_opt = cubec_u32_ne,
      .gt_opt = cubec_u32_gt,
      .lt_opt = cubec_u32_lt,
      .ge_opt = cubec_u32_ge,
      .le_opt = cubec_u32_le,

      .convert = cubec_u32_convert,
  };
  DECLAR_UNSIGNED(32, &u32_opt);
  struct _cubec_type_operator_t u64_opt = {
      .type_to_string = &cubec_numeric_type_to_string,
      .to_string = &cubec_numeric_to_string,
      .add_opt = cubec_u64_add,
      .sub_opt = cubec_u64_sub,
      .div_opt = cubec_u64_div,
      .mul_opt = cubec_u64_mul,
      .mod_opt = cubec_u64_mod,
      .and_opt = cubec_u64_and,
      .or_opt = cubec_u64_or,
      .xor_opt = cubec_u64_xor,
      .shl_opt = cubec_u64_shl,
      .shr_opt = cubec_u64_shr,

      .eq_opt = cubec_u64_eq,
      .ne_opt = cubec_u64_ne,
      .gt_opt = cubec_u64_gt,
      .lt_opt = cubec_u64_lt,
      .ge_opt = cubec_u64_ge,
      .le_opt = cubec_u64_le,

      .convert = cubec_u64_convert,
  };
  DECLAR_UNSIGNED(64, &u64_opt);
  struct _cubec_type_operator_t f32_opt = {
      .type_to_string = &cubec_numeric_type_to_string,
      .to_string = &cubec_numeric_to_string,
      .add_opt = cubec_f32_add,
      .sub_opt = cubec_f32_sub,
      .mul_opt = cubec_f32_mul,
      .div_opt = cubec_f32_div,

      .eq_opt = cubec_f32_eq,
      .ne_opt = cubec_f32_ne,
      .gt_opt = cubec_f32_gt,
      .lt_opt = cubec_f32_lt,
      .ge_opt = cubec_f32_ge,
      .le_opt = cubec_f32_le,

      .convert = cubec_f32_convert,
  };
  DECLAR_FLOAT(32, &f32_opt);
  struct _cubec_type_operator_t f64_opt = {
      .type_to_string = &cubec_numeric_type_to_string,
      .to_string = &cubec_numeric_to_string,
      .add_opt = cubec_f64_add,
      .sub_opt = cubec_f64_sub,
      .mul_opt = cubec_f64_mul,
      .div_opt = cubec_f64_div,

      .eq_opt = cubec_f64_eq,
      .ne_opt = cubec_f64_ne,
      .gt_opt = cubec_f64_gt,
      .lt_opt = cubec_f64_lt,
      .ge_opt = cubec_f64_ge,
      .le_opt = cubec_f64_le,

      .convert = cubec_f64_convert,
  };
  DECLAR_FLOAT(64, &f64_opt);
}
cubec_value_t cubec_create_i8(cubec_context_t ctx, i8_t value, bool mutable,
                              const char *name) {
  cubec_value_t vtype = cubec_context_load(ctx, "i8");
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  return cubec_context_create_value(ctx, type, mutable, &value, name);
}
cubec_value_t cubec_create_i16(cubec_context_t ctx, i16_t value, bool mutable,
                               const char *name) {
  cubec_value_t vtype = cubec_context_load(ctx, "i16");
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  return cubec_context_create_value(ctx, type, mutable, &value, name);
}
cubec_value_t cubec_create_i32(cubec_context_t ctx, i32_t value, bool mutable,
                               const char *name) {
  cubec_value_t vtype = cubec_context_load(ctx, "i32");
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  return cubec_context_create_value(ctx, type, mutable, &value, name);
}
cubec_value_t cubec_create_i64(cubec_context_t ctx, i64_t value, bool mutable,
                               const char *name) {
  cubec_value_t vtype = cubec_context_load(ctx, "i64");
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  return cubec_context_create_value(ctx, type, mutable, &value, name);
}
cubec_value_t cubec_create_u8(cubec_context_t ctx, u8_t value, bool mutable,
                              const char *name) {
  cubec_value_t vtype = cubec_context_load(ctx, "u8");
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  return cubec_context_create_value(ctx, type, mutable, &value, name);
}
cubec_value_t cubec_create_u16(cubec_context_t ctx, u16_t value, bool mutable,
                               const char *name) {
  cubec_value_t vtype = cubec_context_load(ctx, "u16");
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  return cubec_context_create_value(ctx, type, mutable, &value, name);
}
cubec_value_t cubec_create_u32(cubec_context_t ctx, u32_t value, bool mutable,
                               const char *name) {
  cubec_value_t vtype = cubec_context_load(ctx, "u32");
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  return cubec_context_create_value(ctx, type, mutable, &value, name);
}
cubec_value_t cubec_create_u64(cubec_context_t ctx, u64_t value, bool mutable,
                               const char *name) {
  cubec_value_t vtype = cubec_context_load(ctx, "u64");
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  return cubec_context_create_value(ctx, type, mutable, &value, name);
}
cubec_value_t cubec_create_f32(cubec_context_t ctx, f32_t value, bool mutable,
                               const char *name) {
  cubec_value_t vtype = cubec_context_load(ctx, "f32");
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  return cubec_context_create_value(ctx, type, mutable, &value, name);
}
cubec_value_t cubec_create_f64(cubec_context_t ctx, f64_t value, bool mutable,
                               const char *name) {
  cubec_value_t vtype = cubec_context_load(ctx, "f64");
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  return cubec_context_create_value(ctx, type, mutable, &value, name);
}
