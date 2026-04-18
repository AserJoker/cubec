#include "eval/expression_binary.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/numeric.h"
#include "engine/type.h"
#include "engine/value.h"
#include "eval/expression.h"
#include "resolve/expression.h"
#include <stdbool.h>
#include <inttypes.h>

value_t eval_expression_binary(context_t ctx, ast_node_t node) {
  ast_node_t left_node = ast_get_child(node, "left");
  ast_node_t right_node = ast_get_child(node, "right");
  ast_node_t opt = ast_get_child(node, "opt");
  allocator_t allocator = context_get_allocator(ctx);
  if (left_node && right_node) {
    value_t lvalue = eval_expression(ctx, left_node);
    if (value_is_error(lvalue)) {
      return lvalue;
    }
    if (value_is_interrupt(lvalue)) {
      return lvalue;
    }
    if (!value_is_comptime(lvalue)) {
      return create_compile_error(ctx, left_node, "value is not comptime");
    }
    type_t ltype = value_get_type(lvalue);
    if (location_is(opt->loc, "&&")) {
      if (type_get_kind(ltype) != VALUE_TYPE_BOOL) {
        lvalue =
            value_safe_convert(lvalue, ctx, context_load_type(ctx, "bool"));
        if (value_is_error(lvalue)) {
          return convert_compile_error(ctx, left_node, lvalue);
        }
      }
      bool val = *(bool *)value_get_data(lvalue);
      if (val) {
        return lvalue;
      }
    } else if (location_is(opt->loc, "||")) {
      if (type_get_kind(ltype) != VALUE_TYPE_BOOL) {
        lvalue =
            value_safe_convert(lvalue, ctx, context_load_type(ctx, "bool"));
        if (value_is_error(lvalue)) {
          return convert_compile_error(ctx, left_node, lvalue);
        }
      }
      bool val = *(bool *)value_get_data(lvalue);
      if (!val) {
        return lvalue;
      }
    }
    value_t rvalue = eval_expression(ctx, right_node);
    if (value_is_error(rvalue)) {
      return rvalue;
    }
    if (value_is_interrupt(rvalue)) {
      return rvalue;
    }
    if (!value_is_comptime(rvalue)) {
      return create_compile_error(ctx, left_node, "value is not comptime");
    }
    value_t result = NULL;
    if (location_is(opt->loc, "+")) {
      result = value_add(lvalue, ctx, rvalue);
    } else if (location_is(opt->loc, "-")) {
      result = value_sub(lvalue, ctx, rvalue);
    } else if (location_is(opt->loc, "*")) {
      result = value_mul(lvalue, ctx, rvalue);
    } else if (location_is(opt->loc, "/")) {
      result = value_div(lvalue, ctx, rvalue);
    } else if (location_is(opt->loc, "%")) {
      result = value_mod(lvalue, ctx, rvalue);
    } else if (location_is(opt->loc, "&")) {
      result = value_and(lvalue, ctx, rvalue);
    } else if (location_is(opt->loc, "|")) {
      result = value_or(lvalue, ctx, rvalue);
    } else if (location_is(opt->loc, "^")) {
      result = value_xor(lvalue, ctx, rvalue);
    } else if (location_is(opt->loc, "<<")) {
      result = value_shl(lvalue, ctx, rvalue);
    } else if (location_is(opt->loc, ">>")) {
      result = value_shr(lvalue, ctx, rvalue);
    } else if (location_is(opt->loc, "==")) {
      result = value_eq(lvalue, ctx, rvalue);
    } else if (location_is(opt->loc, "!=")) {
      result = value_ne(lvalue, ctx, rvalue);
    } else if (location_is(opt->loc, ">")) {
      result = value_gt(lvalue, ctx, rvalue);
    } else if (location_is(opt->loc, "<")) {
      result = value_lt(lvalue, ctx, rvalue);
    } else if (location_is(opt->loc, ">=")) {
      result = value_ge(lvalue, ctx, rvalue);
    } else if (location_is(opt->loc, "<=")) {
      result = value_le(lvalue, ctx, rvalue);
    } else if (location_is(opt->loc, "&&")) {
      result = value_logical_and(lvalue, ctx, rvalue);
    } else if (location_is(opt->loc, "||")) {
      result = value_logical_or(lvalue, ctx, rvalue);
    } else {
      result = create_error(ctx, "unsupprort binary expression");
    }
    if (value_is_error(result)) {
      result = convert_compile_error(ctx, node, result);
    }
    return result;
  } else if (right_node) {
    if (location_is(opt->loc, "typeof")) {
      value_t value = resolve_expression(ctx, right_node);
      if (value_is_error(value)) {
        return value;
      }
      type_t value_type = value_get_type(value);
      return create_type_value(ctx, value_type, false, NULL);
    } else if (location_is(opt->loc, "sizeof")) {
      value_t value = resolve_expression(ctx, right_node);
      if (value_is_error(value)) {
        return value;
      }
      type_t value_type = value_get_type(value);
      return create_u64(ctx, type_get_size(value_type), false, NULL);
    } else if (location_is(opt->loc, "alignof")) {
      value_t value = resolve_expression(ctx, right_node);
      if (value_is_error(value)) {
        return value;
      }
      type_t value_type = value_get_type(value);
      return create_u64(ctx, type_get_align(value_type), false, NULL);
    } else if (location_is(opt->loc, "&")) {
      if (right_node->type == NODE_TYPE_EXPRESSION_MEMBER) {
        ast_node_t host_node = ast_get_child(right_node, "host");
        ast_node_t field_node = ast_get_child(right_node, "field");
        value_t host = eval_expression(ctx, host_node);
        if (value_is_error(host)) {
          return host;
        }
        if (value_is_interrupt(host)) {
          return host;
        }
        char *field = location_get(field_node->loc, allocator);
        value_t ref = value_member_ref(host, ctx, field);
        allocator_free(allocator, field);
        if (value_is_error(ref)) {
          return convert_compile_error(ctx, node, ref);
        }
        return ref;
      } else if (right_node->type == NODE_TYPE_EXPRESSION_COMPUTE_MEMBER) {
        ast_node_t host_node = ast_get_child(right_node, "host");
        ast_node_t field_node = ast_get_child(right_node, "field");
        value_t host = eval_expression(ctx, host_node);
        if (value_is_error(host)) {
          return host;
        }
        if (value_is_interrupt(host)) {
          return host;
        }
        value_t field = eval_expression(ctx, field_node);
        if (value_is_error(field)) {
          return field;
        }
        if (value_is_interrupt(field)) {
          return field;
        }
        type_t type = value_get_type(host);
        if (value_type_is(field, VALUE_TYPE_STR)) {
          const char *field_name = *(const char **)value_get_data(field);
          value_t val = value_member_ref(host, ctx, field_name);
          if (value_is_error(val)) {
            return convert_compile_error(ctx, node, val);
          }
          return val;
        } else {
          type_t field_type = value_get_type(field);
          type_kind_t kind = type_get_kind(field_type);
          uint64_t idx = 0;
          if (kind >= VALUE_TYPE_INT8 && kind <= VALUE_TYPE_INT64) {
            int64_t i = 0;
            value_t val =
                value_safe_convert(field, ctx, context_load_type(ctx, "i64"));
            i = *(int64_t *)value_get_data(val);
            if (i < 0) {
              return create_compile_error(
                  ctx, field_node,
                  "array index %" PRIdPTR
                  " is before the beginning of the array",
                  i);
            }
            idx = i;
          } else if (kind >= VALUE_TYPE_UINT8 && kind <= VALUE_TYPE_UINT64) {
            value_t val =
                value_safe_convert(field, ctx, context_load_type(ctx, "u64"));
            idx = *(uint64_t *)value_get_data(val);
          } else {
            return create_compile_error(ctx, field_node,
                                        "array subscript is not an integer");
          }
          value_t val = value_index_ref(host, ctx, idx);
          if (value_is_error(val)) {
            return convert_compile_error(ctx, node, val);
          }
          return val;
        }
      }
    }
    value_t value = eval_expression(ctx, right_node);
    if (value_is_error(value)) {
      return value;
    }
    if (value_is_interrupt(value)) {
      return value;
    }
    if (!value_is_comptime(value)) {
      return create_compile_error(ctx, node, "value is not comptime");
    }
    type_t type = value_get_type(value);
    type_kind_t kind = type_get_kind(type);
    type_operator_t op = type_get_operator(type);
    if (location_is(opt->loc, "+")) {
      return value_plus(value, ctx);
    } else if (location_is(opt->loc, "-")) {
      return value_neg(value, ctx);
    } else if (location_is(opt->loc, "!")) {
      return value_logical_not(value, ctx);
    } else if (location_is(opt->loc, "*")) {
      return value_unref(value, ctx);
    } else if (location_is(opt->loc, "&")) {
      return value_ref(value, ctx);
    } else if (location_is(opt->loc, "try")) {
      return value_try(value, ctx);
    }
  }
  return create_compile_error(ctx, node, "unsupprort binary expression");
}