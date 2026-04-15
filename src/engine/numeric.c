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
  context_create_type(ctx, CUBEC_VALUE_TYPE_INT##size, sizeof(i##size##_t),    \
                      sizeof(i##size##_t), NULL, opt, "i" #size)
#define DECLAR_UNSIGNED(size, opt)                                             \
  context_create_type(ctx, CUBEC_VALUE_TYPE_UINT##size, sizeof(u##size##_t),   \
                      sizeof(u##size##_t), NULL, opt, "u" #size)
#define DECLAR_FLOAT(size, opt)                                                \
  context_create_type(ctx, CUBEC_VALUE_TYPE_FLOAT##size, sizeof(f##size##_t),  \
                      sizeof(f##size##_t), NULL, opt, "f" #size)

static char *numeric_type_to_string(type_t self, allocator_t allocator) {
  type_kind_t kind = type_get_kind(self);
  switch (kind) {
  case CUBEC_VALUE_TYPE_INT8:
    return create_cstring(allocator, "i8");
  case CUBEC_VALUE_TYPE_INT16:
    return create_cstring(allocator, "i16");
  case CUBEC_VALUE_TYPE_INT32:
    return create_cstring(allocator, "i32");
  case CUBEC_VALUE_TYPE_INT64:
    return create_cstring(allocator, "i64");
  case CUBEC_VALUE_TYPE_UINT8:
    return create_cstring(allocator, "u8");
  case CUBEC_VALUE_TYPE_UINT16:
    return create_cstring(allocator, "u16");
  case CUBEC_VALUE_TYPE_UINT32:
    return create_cstring(allocator, "u32");
  case CUBEC_VALUE_TYPE_UINT64:
    return create_cstring(allocator, "u64");
  case CUBEC_VALUE_TYPE_FLOAT32:
    return create_cstring(allocator, "f32");
  case CUBEC_VALUE_TYPE_FLOAT64:
    return create_cstring(allocator, "f64");
  default:
    break;
  }
  return NULL;
}
static value_t numeric_to_string(value_t self, context_t ctx) {
  type_t type = value_get_type(self);
  type_kind_t kind = type_get_kind(type);
  char str[32] = {0};
  void *data = value_get_data(self);
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
  return create_str(ctx, str, NULL);
}

#define DECLAR_BINARY_OPT(type, rtype, name, opt)                              \
  static value_t type##_##name(value_t self, context_t ctx, value_t another) { \
    if (!value_get_data(self) || !value_get_data(another)) {                   \
      value_t vtype = context_load(ctx, #rtype);                               \
      type_t type = *(type_t *)value_get_data(vtype);                          \
      return context_create_value(ctx, type, false, NULL, NULL);               \
    }                                                                          \
    type##_t left = *(type##_t *)value_get_data(self);                         \
    type##_t right = *(type##_t *)value_get_data(another);                     \
    return create_##rtype(ctx, left opt right, true, NULL);                    \
  }

#define DECLAR_SINGLE_OPT(type, rtype, name, opt)                              \
  static value_t type##_##name(value_t self, context_t ctx) {                  \
    if (!value_get_data(self)) {                                               \
      value_t vtype = context_load(ctx, #rtype);                               \
      type_t type = *(type_t *)value_get_data(vtype);                          \
      return context_create_value(ctx, type, false, NULL, NULL);               \
    }                                                                          \
    type##_t left = *(type##_t *)value_get_data(self);                         \
    return create_##rtype(ctx, opt left, true, NULL);                          \
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
DECLAR_SINGLE_OPT(i8, i8, plus, +);
DECLAR_SINGLE_OPT(i8, i8, neg, -);
DECLAR_SINGLE_OPT(i8, i8, bitwise_not, ~);

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
DECLAR_SINGLE_OPT(i16, i16, plus, +);
DECLAR_SINGLE_OPT(i16, i16, neg, -);
DECLAR_SINGLE_OPT(i16, i16, bitwise_not, ~);

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
DECLAR_SINGLE_OPT(i32, i32, plus, +);
DECLAR_SINGLE_OPT(i32, i32, neg, -);
DECLAR_SINGLE_OPT(i32, i32, bitwise_not, ~);

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
DECLAR_SINGLE_OPT(i64, i64, plus, +);
DECLAR_SINGLE_OPT(i64, i64, neg, -);
DECLAR_SINGLE_OPT(i64, i64, bitwise_not, ~);

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
DECLAR_SINGLE_OPT(u8, u8, plus, +);
DECLAR_SINGLE_OPT(u8, u8, bitwise_not, ~);

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
DECLAR_SINGLE_OPT(u16, u16, plus, +);
DECLAR_SINGLE_OPT(u16, u16, bitwise_not, ~);

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
DECLAR_SINGLE_OPT(u32, u32, plus, +);
DECLAR_SINGLE_OPT(u32, u32, bitwise_not, ~);

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
DECLAR_SINGLE_OPT(u64, u64, plus, +);
DECLAR_SINGLE_OPT(u64, u64, bitwise_not, ~);

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
DECLAR_SINGLE_OPT(f32, f32, plus, +);
DECLAR_SINGLE_OPT(f32, f32, neg, -);

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
DECLAR_SINGLE_OPT(f64, f64, plus, +);
DECLAR_SINGLE_OPT(f64, f64, neg, -);

DECLAR_BINARY_OPT(f64, boolean, eq, ==);
DECLAR_BINARY_OPT(f64, boolean, ne, !=);
DECLAR_BINARY_OPT(f64, boolean, gt, >);
DECLAR_BINARY_OPT(f64, boolean, lt, <);
DECLAR_BINARY_OPT(f64, boolean, ge, >=);
DECLAR_BINARY_OPT(f64, boolean, le, <=);

#define DECLAR_CONVERT(type)                                                   \
  static value_t type##_convert(value_t self, context_t ctx, type_t type) {    \
    type##_t *value = (type##_t *)value_get_data(self);                        \
    type_kind_t kind = type_get_kind(type);                                    \
    switch (kind) {                                                            \
    case CUBEC_VALUE_TYPE_BOOL:                                                \
      if (value) {                                                             \
        return create_boolean(ctx, *value, false, NULL);                       \
      } else {                                                                 \
        return context_create_value(ctx, type, false, NULL, NULL);             \
      }                                                                        \
    case CUBEC_VALUE_TYPE_INT8:                                                \
      if (value) {                                                             \
        return create_i8(ctx, *value, false, NULL);                            \
      } else {                                                                 \
        return context_create_value(ctx, type, false, NULL, NULL);             \
      }                                                                        \
    case CUBEC_VALUE_TYPE_INT16:                                               \
      if (value) {                                                             \
        return create_i16(ctx, *value, false, NULL);                           \
      } else {                                                                 \
        return context_create_value(ctx, type, false, NULL, NULL);             \
      }                                                                        \
    case CUBEC_VALUE_TYPE_INT32:                                               \
      if (value) {                                                             \
        return create_i32(ctx, *value, false, NULL);                           \
      } else {                                                                 \
        return context_create_value(ctx, type, false, NULL, NULL);             \
      }                                                                        \
    case CUBEC_VALUE_TYPE_INT64:                                               \
      if (value) {                                                             \
        return create_i64(ctx, *value, false, NULL);                           \
      } else {                                                                 \
        return context_create_value(ctx, type, false, NULL, NULL);             \
      }                                                                        \
    case CUBEC_VALUE_TYPE_UINT8:                                               \
      if (value) {                                                             \
        return create_u8(ctx, *value, false, NULL);                            \
      } else {                                                                 \
        return context_create_value(ctx, type, false, NULL, NULL);             \
      }                                                                        \
    case CUBEC_VALUE_TYPE_UINT16:                                              \
      if (value) {                                                             \
        return create_u16(ctx, *value, false, NULL);                           \
      } else {                                                                 \
        return context_create_value(ctx, type, false, NULL, NULL);             \
      }                                                                        \
    case CUBEC_VALUE_TYPE_UINT32:                                              \
      if (value) {                                                             \
        return create_u32(ctx, *value, false, NULL);                           \
      } else {                                                                 \
        return context_create_value(ctx, type, false, NULL, NULL);             \
      }                                                                        \
    case CUBEC_VALUE_TYPE_UINT64:                                              \
      if (value) {                                                             \
        return create_u64(ctx, *value, false, NULL);                           \
      } else {                                                                 \
        return context_create_value(ctx, type, false, NULL, NULL);             \
      }                                                                        \
    case CUBEC_VALUE_TYPE_FLOAT32:                                             \
      if (value) {                                                             \
        return create_f32(ctx, *value, false, NULL);                           \
      } else {                                                                 \
        return context_create_value(ctx, type, false, NULL, NULL);             \
      }                                                                        \
    case CUBEC_VALUE_TYPE_FLOAT64:                                             \
      if (value) {                                                             \
        return create_f64(ctx, *value, false, NULL);                           \
      } else {                                                                 \
        return context_create_value(ctx, type, false, NULL, NULL);             \
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
void init_numeric_type(context_t ctx) {
  struct _type_operator_t i8_opt = {
      .type_to_string = &numeric_type_to_string,
      .to_string = &numeric_to_string,
      .add_opt = i8_add,
      .sub_opt = i8_sub,
      .div_opt = i8_div,
      .mul_opt = i8_mul,
      .mod_opt = i8_mod,
      .and_opt = i8_and,
      .or_opt = i8_or,
      .xor_opt = i8_xor,
      .shl_opt = i8_shl,
      .shr_opt = i8_shr,
      .bitwise_not_opt = i8_bitwise_not,

      .eq_opt = i8_eq,
      .ne_opt = i8_ne,
      .gt_opt = i8_gt,
      .lt_opt = i8_lt,
      .ge_opt = i8_ge,
      .le_opt = i8_le,
      .plus_opt = i8_plus,
      .neg_opt = i8_neg,

      .convert = i8_convert,
  };
  DECLAR_INTEGER(8, &i8_opt);
  struct _type_operator_t i16_opt = {
      .type_to_string = &numeric_type_to_string,
      .to_string = &numeric_to_string,
      .add_opt = i16_add,
      .sub_opt = i16_sub,
      .div_opt = i16_div,
      .mul_opt = i16_mul,
      .mod_opt = i16_mod,
      .and_opt = i16_and,
      .or_opt = i16_or,
      .xor_opt = i16_xor,
      .shl_opt = i16_shl,
      .shr_opt = i16_shr,
      .bitwise_not_opt = i16_bitwise_not,

      .eq_opt = i16_eq,
      .ne_opt = i16_ne,
      .gt_opt = i16_gt,
      .lt_opt = i16_lt,
      .ge_opt = i16_ge,
      .le_opt = i16_le,
      .plus_opt = i16_plus,
      .neg_opt = i16_neg,

      .convert = i16_convert,
  };
  DECLAR_INTEGER(16, &i16_opt);
  struct _type_operator_t i32_opt = {
      .type_to_string = &numeric_type_to_string,
      .to_string = &numeric_to_string,
      .add_opt = i32_add,
      .sub_opt = i32_sub,
      .div_opt = i32_div,
      .mul_opt = i32_mul,
      .mod_opt = i32_mod,
      .and_opt = i32_and,
      .or_opt = i32_or,
      .xor_opt = i32_xor,
      .shl_opt = i32_shl,
      .shr_opt = i32_shr,
      .bitwise_not_opt = i32_bitwise_not,

      .eq_opt = i32_eq,
      .ne_opt = i32_ne,
      .gt_opt = i32_gt,
      .lt_opt = i32_lt,
      .ge_opt = i32_ge,
      .le_opt = i32_le,
      .plus_opt = i32_plus,
      .neg_opt = i32_neg,

      .convert = i32_convert,
  };
  DECLAR_INTEGER(32, &i32_opt);
  struct _type_operator_t i64_opt = {
      .type_to_string = &numeric_type_to_string,
      .to_string = &numeric_to_string,
      .add_opt = i64_add,
      .sub_opt = i64_sub,
      .div_opt = i64_div,
      .mul_opt = i64_mul,
      .mod_opt = i64_mod,
      .and_opt = i64_and,
      .or_opt = i64_or,
      .xor_opt = i64_xor,
      .shl_opt = i64_shl,
      .shr_opt = i64_shr,
      .bitwise_not_opt = i64_bitwise_not,

      .eq_opt = i64_eq,
      .ne_opt = i64_ne,
      .gt_opt = i64_gt,
      .lt_opt = i64_lt,
      .ge_opt = i64_ge,
      .le_opt = i64_le,
      .plus_opt = i64_plus,
      .neg_opt = i64_neg,

      .convert = i64_convert,
  };
  DECLAR_INTEGER(64, &i64_opt);
  struct _type_operator_t u8_opt = {
      .type_to_string = &numeric_type_to_string,
      .to_string = &numeric_to_string,
      .add_opt = u8_add,
      .sub_opt = u8_sub,
      .div_opt = u8_div,
      .mul_opt = u8_mul,
      .mod_opt = u8_mod,
      .and_opt = u8_and,
      .or_opt = u8_or,
      .xor_opt = u8_xor,
      .shl_opt = u8_shl,
      .shr_opt = u8_shr,
      .bitwise_not_opt = u8_bitwise_not,

      .eq_opt = u8_eq,
      .ne_opt = u8_ne,
      .gt_opt = u8_gt,
      .lt_opt = u8_lt,
      .ge_opt = u8_ge,
      .le_opt = u8_le,
      .plus_opt = u8_plus,

      .convert = u8_convert,
  };
  DECLAR_UNSIGNED(8, &u8_opt);
  struct _type_operator_t u16_opt = {
      .type_to_string = &numeric_type_to_string,
      .to_string = &numeric_to_string,
      .add_opt = u16_add,
      .sub_opt = u16_sub,
      .div_opt = u16_div,
      .mul_opt = u16_mul,
      .mod_opt = u16_mod,
      .and_opt = u16_and,
      .or_opt = u16_or,
      .xor_opt = u16_xor,
      .shl_opt = u16_shl,
      .shr_opt = u16_shr,
      .bitwise_not_opt = u16_bitwise_not,

      .eq_opt = u16_eq,
      .ne_opt = u16_ne,
      .gt_opt = u16_gt,
      .lt_opt = u16_lt,
      .ge_opt = u16_ge,
      .le_opt = u16_le,
      .plus_opt = u16_plus,

      .convert = u16_convert,
  };
  DECLAR_UNSIGNED(16, &u16_opt);
  struct _type_operator_t u32_opt = {
      .type_to_string = &numeric_type_to_string,
      .to_string = &numeric_to_string,
      .add_opt = u32_add,
      .sub_opt = u32_sub,
      .div_opt = u32_div,
      .mul_opt = u32_mul,
      .mod_opt = u32_mod,
      .and_opt = u32_and,
      .or_opt = u32_or,
      .xor_opt = u32_xor,
      .shl_opt = u32_shl,
      .shr_opt = u32_shr,
      .bitwise_not_opt = u32_bitwise_not,

      .eq_opt = u32_eq,
      .ne_opt = u32_ne,
      .gt_opt = u32_gt,
      .lt_opt = u32_lt,
      .ge_opt = u32_ge,
      .le_opt = u32_le,
      .plus_opt = u32_plus,

      .convert = u32_convert,
  };
  DECLAR_UNSIGNED(32, &u32_opt);
  struct _type_operator_t u64_opt = {
      .type_to_string = &numeric_type_to_string,
      .to_string = &numeric_to_string,
      .add_opt = u64_add,
      .sub_opt = u64_sub,
      .div_opt = u64_div,
      .mul_opt = u64_mul,
      .mod_opt = u64_mod,
      .and_opt = u64_and,
      .or_opt = u64_or,
      .xor_opt = u64_xor,
      .shl_opt = u64_shl,
      .shr_opt = u64_shr,
      .bitwise_not_opt = u64_bitwise_not,

      .eq_opt = u64_eq,
      .ne_opt = u64_ne,
      .gt_opt = u64_gt,
      .lt_opt = u64_lt,
      .ge_opt = u64_ge,
      .le_opt = u64_le,
      .plus_opt = u64_plus,

      .convert = u64_convert,
  };
  DECLAR_UNSIGNED(64, &u64_opt);
  struct _type_operator_t f32_opt = {
      .type_to_string = &numeric_type_to_string,
      .to_string = &numeric_to_string,
      .add_opt = f32_add,
      .sub_opt = f32_sub,
      .mul_opt = f32_mul,
      .div_opt = f32_div,

      .eq_opt = f32_eq,
      .ne_opt = f32_ne,
      .gt_opt = f32_gt,
      .lt_opt = f32_lt,
      .ge_opt = f32_ge,
      .le_opt = f32_le,
      .plus_opt = f32_plus,

      .convert = f32_convert,
  };
  DECLAR_FLOAT(32, &f32_opt);
  struct _type_operator_t f64_opt = {
      .type_to_string = &numeric_type_to_string,
      .to_string = &numeric_to_string,
      .add_opt = f64_add,
      .sub_opt = f64_sub,
      .mul_opt = f64_mul,
      .div_opt = f64_div,

      .eq_opt = f64_eq,
      .ne_opt = f64_ne,
      .gt_opt = f64_gt,
      .lt_opt = f64_lt,
      .ge_opt = f64_ge,
      .le_opt = f64_le,
      .plus_opt = f64_plus,

      .convert = f64_convert,
  };
  DECLAR_FLOAT(64, &f64_opt);
}
value_t create_i8(context_t ctx, i8_t value, bool mutable, const char *name) {
  value_t vtype = context_load(ctx, "i8");
  type_t type = *(type_t *)value_get_data(vtype);
  return context_create_value(ctx, type, mutable, &value, name);
}
value_t create_i16(context_t ctx, i16_t value, bool mutable, const char *name) {
  value_t vtype = context_load(ctx, "i16");
  type_t type = *(type_t *)value_get_data(vtype);
  return context_create_value(ctx, type, mutable, &value, name);
}
value_t create_i32(context_t ctx, i32_t value, bool mutable, const char *name) {
  value_t vtype = context_load(ctx, "i32");
  type_t type = *(type_t *)value_get_data(vtype);
  return context_create_value(ctx, type, mutable, &value, name);
}
value_t create_i64(context_t ctx, i64_t value, bool mutable, const char *name) {
  value_t vtype = context_load(ctx, "i64");
  type_t type = *(type_t *)value_get_data(vtype);
  return context_create_value(ctx, type, mutable, &value, name);
}
value_t create_u8(context_t ctx, u8_t value, bool mutable, const char *name) {
  value_t vtype = context_load(ctx, "u8");
  type_t type = *(type_t *)value_get_data(vtype);
  return context_create_value(ctx, type, mutable, &value, name);
}
value_t create_u16(context_t ctx, u16_t value, bool mutable, const char *name) {
  value_t vtype = context_load(ctx, "u16");
  type_t type = *(type_t *)value_get_data(vtype);
  return context_create_value(ctx, type, mutable, &value, name);
}
value_t create_u32(context_t ctx, u32_t value, bool mutable, const char *name) {
  value_t vtype = context_load(ctx, "u32");
  type_t type = *(type_t *)value_get_data(vtype);
  return context_create_value(ctx, type, mutable, &value, name);
}
value_t create_u64(context_t ctx, u64_t value, bool mutable, const char *name) {
  value_t vtype = context_load(ctx, "u64");
  type_t type = *(type_t *)value_get_data(vtype);
  return context_create_value(ctx, type, mutable, &value, name);
}
value_t create_f32(context_t ctx, f32_t value, bool mutable, const char *name) {
  value_t vtype = context_load(ctx, "f32");
  type_t type = *(type_t *)value_get_data(vtype);
  return context_create_value(ctx, type, mutable, &value, name);
}
value_t create_f64(context_t ctx, f64_t value, bool mutable, const char *name) {
  value_t vtype = context_load(ctx, "f64");
  type_t type = *(type_t *)value_get_data(vtype);
  return context_create_value(ctx, type, mutable, &value, name);
}
