#include "run/run.h"
#include "cubec/literal_numeric.h"
#include "engine/vm.h"
#include "engine/value.h"
#include "engine/integer_type.h"
#include "engine/float_type.h"
#include "engine/exception_type.h"
#include "core/string.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

static type_t get_numeric_type(vm_t vm,
                               cubec_literal_numeric_type_t ntype) {
  switch (ntype) {
  case CUBEC_LITERAL_NUMERIC_TYPE_I8:
    return (type_t)value_get_data(vm_get_i8_type(vm));
  case CUBEC_LITERAL_NUMERIC_TYPE_I16:
    return (type_t)value_get_data(vm_get_i16_type(vm));
  case CUBEC_LITERAL_NUMERIC_TYPE_I32:
    return (type_t)value_get_data(vm_get_i32_type(vm));
  case CUBEC_LITERAL_NUMERIC_TYPE_I64:
    return (type_t)value_get_data(vm_get_i64_type(vm));
  case CUBEC_LITERAL_NUMERIC_TYPE_U8:
    return (type_t)value_get_data(vm_get_u8_type(vm));
  case CUBEC_LITERAL_NUMERIC_TYPE_U16:
    return (type_t)value_get_data(vm_get_u16_type(vm));
  case CUBEC_LITERAL_NUMERIC_TYPE_U32:
    return (type_t)value_get_data(vm_get_u32_type(vm));
  case CUBEC_LITERAL_NUMERIC_TYPE_U64:
    return (type_t)value_get_data(vm_get_u64_type(vm));
  case CUBEC_LITERAL_NUMERIC_TYPE_F16:
    return (type_t)value_get_data(vm_get_f16_type(vm));
  case CUBEC_LITERAL_NUMERIC_TYPE_F32:
    return (type_t)value_get_data(vm_get_f32_type(vm));
  case CUBEC_LITERAL_NUMERIC_TYPE_F64:
    return (type_t)value_get_data(vm_get_f64_type(vm));
  default:
    return (type_t)value_get_data(vm_get_i32_type(vm));
  }
}

value_t run_literal_numeric(context_t ctx, node_t node, bool shadow) {
  cubec_literal_numeric_t lit = (cubec_literal_numeric_t)node;
  vm_t vm = ctx->vm;
  const char *str = string_get(lit->value);

  type_t type = get_numeric_type(vm, lit->numeric_type);

  if (shadow)
    return vm_create_value_shadow(vm, type, NULL, true);

  /* Parse and create value based on explicit type suffix or default */
  switch (lit->numeric_type) {
  /* signed integers */
  case CUBEC_LITERAL_NUMERIC_TYPE_I8:
    return create_i8_value(vm, (int8_t)strtol(str, NULL, 0));
  case CUBEC_LITERAL_NUMERIC_TYPE_I16:
    return create_i16_value(vm, (int16_t)strtol(str, NULL, 0));
  case CUBEC_LITERAL_NUMERIC_TYPE_I32:
    return create_i32_value(vm, (int32_t)strtol(str, NULL, 0));
  case CUBEC_LITERAL_NUMERIC_TYPE_I64:
    return create_i64_value(vm, (int64_t)strtoll(str, NULL, 0));
  /* unsigned integers */
  case CUBEC_LITERAL_NUMERIC_TYPE_U8:
    return create_u8_value(vm, (uint8_t)strtoul(str, NULL, 0));
  case CUBEC_LITERAL_NUMERIC_TYPE_U16:
    return create_u16_value(vm, (uint16_t)strtoul(str, NULL, 0));
  case CUBEC_LITERAL_NUMERIC_TYPE_U32:
    return create_u32_value(vm, (uint32_t)strtoul(str, NULL, 0));
  case CUBEC_LITERAL_NUMERIC_TYPE_U64:
    return create_u64_value(vm, (uint64_t)strtoull(str, NULL, 0));
  /* floats */
  case CUBEC_LITERAL_NUMERIC_TYPE_F16: {
    float f = strtof(str, NULL);
    uint16_t bits;
    memcpy(&bits, &f, sizeof(bits));
    return create_f16_value(vm, bits);
  }
  case CUBEC_LITERAL_NUMERIC_TYPE_F32:
    return create_f32_value(vm, strtof(str, NULL));
  case CUBEC_LITERAL_NUMERIC_TYPE_F64:
    return create_f64_value(vm, strtod(str, NULL));
  /* default: integer literal without suffix */
  default:
    if (lit->kind == CUBEC_LITERAL_NUMERIC_KIND_FLOAT)
      return create_f64_value(vm, strtod(str, NULL));
    return create_i32_value(vm, (int32_t)strtol(str, NULL, 0));
  }
}
