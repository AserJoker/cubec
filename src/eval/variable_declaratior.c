#include "eval/variable_declaratior.h"
#include "ast/expression_group.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/numeric.h"
#include "engine/type.h"
#include "engine/value.h"
#include "eval/expression.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

cubec_value_t cubec_eval_variable_declaratior(cubec_context_t ctx,
                                              cubec_ast_node_t node,
                                              cubec_ast_node_t kind) {
  cubec_ast_node_t identifier = cubec_ast_get_child(node, "identifier");
  cubec_ast_node_t type = cubec_ast_get_child(node, "type");
  cubec_ast_node_t initialize = cubec_ast_get_child(node, "initialize");
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  initialize = cubec_ast_unwrap_group(initialize);
  if (initialize->type == CUBEC_NODE_TYPE_INITIALIZE_LIST) {
    cubec_ast_node_t itype = cubec_ast_get_child(initialize, "type");
    if (!itype) {
      if (!type) {
        return cubec_create_compile_error(ctx, node,
                                          "Missing type for initialize list");
      }
      type = cubec_ast_move_child(node, "type");
      cubec_ast_add_child(allocator, initialize, "type", type);
      type = NULL;
    }
  }
  cubec_type_t value_type = NULL;
  if (type) {
    cubec_value_t vtype = cubec_eval_expression(ctx, type);
    if (cubec_value_is_error(vtype)) {
      return vtype;
    }
    cubec_type_t t = cubec_value_get_type(vtype);
    if (cubec_type_get_kind(t) != CUBEC_VALUE_TYPE_TYPE) {
      return cubec_create_compile_error(ctx, type, "value is not a type");
    }
    if (!cubec_value_get_data(vtype)) {
      return cubec_create_compile_error(ctx, type, "value is not comptime");
    }
    value_type = *(cubec_type_t *)cubec_value_get_data(vtype);
  }
  cubec_value_t value = NULL;
  if (value_type && cubec_type_get_kind(value_type) >= CUBEC_VALUE_TYPE_INT8 &&
      cubec_type_get_kind(value_type) <= CUBEC_VALUE_TYPE_FLOAT64) {
    if (initialize->type == CUBEC_NODE_TYPE_LITERAL_NUMERIC) {
      bool is_floating = false;
      for (const char *ch = initialize->loc.begin.offset;
           ch != initialize->loc.end.offset; ch++) {
        if (*ch == '.' || *ch == 'e' || *ch == 'E') {
          is_floating = true;
        }
      }
      char *s = cubec_location_get(initialize->loc, allocator);
      if (is_floating) {
        double val = 0;
        cubec_cstring_to_dec(s, &val);
        cubec_allocator_free(allocator, s);
        if (cubec_type_get_kind(value_type) == CUBEC_VALUE_TYPE_FLOAT32) {
          value = cubec_create_f32(ctx, val, false, NULL);
        } else if (cubec_type_get_kind(value_type) ==
                   CUBEC_VALUE_TYPE_FLOAT64) {
          value = cubec_create_f64(ctx, val, false, NULL);
        } else {
          char *type_name = cubec_type_to_string(value_type, allocator);
          value = cubec_create_compile_error(
              ctx, initialize, "cannot assigment value %g to type '%s'", val,
              type_name);
          cubec_allocator_free(allocator, type_name);
        }
      } else {
        const char *str = s;
        int radix = 10;
        if (*s == '0' && (*(s + 1) == 'x' || *(s + 1) == 'X')) {
          radix = 16;
          str += 2;
        }
        if (*s == '0' && (*(s + 1) == 'o' || *(s + 1) == 'O')) {
          radix = 8;
          str += 2;
        }
        if (*s == '0' && (*(s + 1) == 'b' || *(s + 1) == 'B')) {
          radix = 2;
          str += 2;
        }
        uint64_t val = 0;
        cubec_cstring_to_int(str, &val, 10);
        cubec_allocator_free(allocator, s);
        if (cubec_type_get_kind(value_type) == CUBEC_VALUE_TYPE_INT8) {
          if (val > INT8_MAX) {
            value = cubec_create_compile_error(
                ctx, initialize, "value %" PRIuPTR " is out of range for int8",
                val);
          } else {
            value = cubec_create_i8(ctx, val, false, NULL);
          }
        } else if (cubec_type_get_kind(value_type) == CUBEC_VALUE_TYPE_INT16) {
          if (val > INT16_MAX) {
            value = cubec_create_compile_error(
                ctx, initialize, "value %" PRIuPTR " is out of range for int16",
                val);
          } else {
            value = cubec_create_i16(ctx, val, false, NULL);
          }
        } else if (cubec_type_get_kind(value_type) == CUBEC_VALUE_TYPE_INT32) {
          if (val > INT32_MAX) {
            value = cubec_create_compile_error(
                ctx, initialize, "value %" PRIuPTR " is out of range for int32",
                val);
          } else {
            value = cubec_create_i32(ctx, val, false, NULL);
          }
        } else if (cubec_type_get_kind(value_type) == CUBEC_VALUE_TYPE_INT64) {
          if (val > INT64_MAX) {
            value = cubec_create_compile_error(
                ctx, initialize, "value %" PRIuPTR " is out of range for int64",
                val);
          } else {
            value = cubec_create_i64(ctx, val, false, NULL);
          }
        } else if (cubec_type_get_kind(value_type) == CUBEC_VALUE_TYPE_UINT8) {
          if (val > UINT8_MAX) {
            value = cubec_create_compile_error(
                ctx, initialize, "value %" PRIuPTR " is out of range for uint8",
                val);
          } else {
            value = cubec_create_u8(ctx, val, false, NULL);
          }
        } else if (cubec_type_get_kind(value_type) == CUBEC_VALUE_TYPE_UINT16) {
          if (val > UINT16_MAX) {
            value = cubec_create_compile_error(
                ctx, initialize,
                "value %" PRIuPTR " is out of range for uint16", val);
          } else {
            value = cubec_create_u16(ctx, val, false, NULL);
          }
        } else if (cubec_type_get_kind(value_type) == CUBEC_VALUE_TYPE_UINT32) {
          if (val > UINT32_MAX) {
            value = cubec_create_compile_error(
                ctx, initialize,
                "value %" PRIuPTR " is out of range for uint32", val);
          } else {
            value = cubec_create_u32(ctx, val, false, NULL);
          }
        } else if (cubec_type_get_kind(value_type) == CUBEC_VALUE_TYPE_UINT64) {
          value = cubec_create_u64(ctx, val, false, NULL);
        } else {
          char *type_name = cubec_type_to_string(value_type, allocator);
          value = cubec_create_compile_error(ctx, initialize,
                                             "cannot assigment value %" PRIuPTR
                                             " to type '%s'",
                                             val, type_name);
          cubec_allocator_free(allocator, type_name);
        }
      }
    }
  }
  if (!value) {
    value = cubec_eval_expression(ctx, initialize);
  }
  if (cubec_value_is_error(value)) {
    return value;
  }
  if (value_type) {
    cubec_type_t current_type = cubec_value_get_type(value);
    if (!cubec_type_is_equal(value_type, current_type)) {
      value = cubec_value_safe_convert(value, ctx, value_type);
      if (cubec_value_is_error(value)) {
        return cubec_convert_compile_error(ctx, node, value);
      }
    }
  } else {
    value_type = cubec_value_get_type(value);
  }
  void *data = cubec_value_get_data(value);
  if (cubec_location_is(kind->loc, "comptime") && !data) {
    return cubec_create_compile_error(ctx, initialize, "value is not comptime");
  }
  char *c_id = cubec_location_get(identifier->loc, allocator);
  bool mutable = cubec_location_is(kind->loc, "let");
  value = cubec_context_create_value(ctx, value_type, mutable, data, c_id);
  cubec_allocator_free(allocator, c_id);
  if (cubec_value_is_error(value)) {
    return cubec_convert_compile_error(ctx, node, value);
  }
  return value;
}