#include "eval/expression_binary.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/numeric.h"
#include "engine/type.h"
#include "engine/value.h"
#include "eval/expression.h"
#include "resolve/expression.h"
#include <stdbool.h>

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
    value_t result = NULL;
    if (location_is(opt->loc, "+")) {
      result = value_plus(value, ctx);
    } else if (location_is(opt->loc, "-")) {
      result = value_neg(value, ctx);
    } else if (location_is(opt->loc, "!")) {
      result = value_logical_not(value, ctx);
    } else if (location_is(opt->loc, "&")) {
      result = value_ref(value, ctx);
    } else if (location_is(opt->loc, "*")) {
      result = value_unref(value, ctx);
    } else if (location_is(opt->loc, "try")) {
      return value_try(value, ctx);
    }
  }
  return create_compile_error(ctx, node, "unsupprort binary expression");
}