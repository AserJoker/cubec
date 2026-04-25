#include "resolve/expression_binary.h"
#include "ast/node.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/type.h"
#include "engine/value.h"
#include "resolve/expression.h"
#include <stdbool.h>

static value_t resolve_convert_expression(context_t ctx, ast_node_t node,
                                          type_t type) {
  value_t val = resolve_expression(ctx, node);
  if (value_is_error(val)) {
    return val;
  }
  if (value_is_interrupt(val)) {
    return val;
  }
  val = value_safe_convert(val, ctx, type);
  if (value_is_error(val)) {
    return convert_comptime_error(ctx, node, val);
  }
  return val;
}

static value_t resolve_expression_comptime_logical_and(context_t ctx,
                                                       ast_node_t left_node,
                                                       ast_node_t right_node) {
  type_t bool_t = context_load_type(ctx, "bool");
  value_t left = resolve_convert_expression(ctx, left_node, bool_t);
  if (value_is_error(left)) {
    return left;
  }
  if (value_is_interrupt(left)) {
    return left;
  }
  if (!value_is_comptime(left)) {
    return create_comptime_error(ctx, left_node, "value is not comptime");
  }
  bool left_value = *(bool *)value_get_data(left);
  if (left_value) {
    bool comptime = context_set_comptime(ctx, false);
    value_t right = resolve_convert_expression(ctx, right_node, bool_t);
    context_set_comptime(ctx, comptime);
    if (value_is_error(right)) {
      return right;
    }
    if (value_is_interrupt(right)) {
      return right;
    }
    return left;
  } else {
    value_t right = resolve_convert_expression(ctx, right_node, bool_t);
    if (value_is_error(right)) {
      return right;
    }
    if (value_is_interrupt(right)) {
      return right;
    }
    return right;
  }
}

static value_t resolve_expression_comptime_logical_or(context_t ctx,
                                                      ast_node_t left_node,
                                                      ast_node_t right_node) {
  type_t bool_t = context_load_type(ctx, "bool");
  value_t left = resolve_convert_expression(ctx, left_node, bool_t);
  if (value_is_error(left)) {
    return left;
  }
  if (value_is_interrupt(left)) {
    return left;
  }
  if (!value_is_comptime(left)) {
    return create_comptime_error(ctx, left_node, "value is not comptime");
  }
  bool left_value = *(bool *)value_get_data(left);
  if (!left_value) {
    bool comptime = context_set_comptime(ctx, false);
    value_t right = resolve_convert_expression(ctx, right_node, bool_t);
    context_set_comptime(ctx, comptime);
    if (value_is_error(right)) {
      return right;
    }
    if (value_is_interrupt(right)) {
      return right;
    }
    return left;
  } else {
    value_t right = resolve_convert_expression(ctx, right_node, bool_t);
    if (value_is_error(right)) {
      return right;
    }
    if (value_is_interrupt(right)) {
      return right;
    }
    return right;
  }
}

value_t resolve_expression_binary(context_t ctx, ast_node_t node) {
  allocator_t allocator = context_get_allocator(ctx);
  ast_node_t left_node = ast_get_child(node, "left");
  ast_node_t right_node = ast_get_child(node, "right");
  ast_node_t opt = ast_get_child(node, "opt");
  value_t result = NULL;
  if (left_node && right_node) {
    if (context_is_comptime(ctx)) {
      if (location_is(opt->loc, "&&")) {
        return resolve_expression_comptime_logical_and(ctx, left_node,
                                                       right_node);
      } else if (location_is(opt->loc, "||")) {
        return resolve_expression_comptime_logical_or(ctx, left_node,
                                                      right_node);
      }
    }
    value_t left = resolve_expression(ctx, left_node);
    if (value_is_error(left)) {
      return left;
    }
    if (value_is_interrupt(left)) {
      return left;
    }
    value_t right = resolve_expression(ctx, right_node);
    if (value_is_error(right)) {
      return right;
    }
    if (value_is_interrupt(right)) {
      return right;
    }
    if (value_is_writer(left) && !value_is_writer(right)) {
      ast_remove_child(node, "left");
      left_node = create_ast_value_node(allocator, left);
      ast_add_child(allocator, node, "left", left_node);
    } else if (value_is_writer(right) && !value_is_writer(left)) {
      ast_remove_child(node, "right");
      right_node = create_ast_value_node(allocator, right);
      ast_add_child(allocator, node, "right", right_node);
    }
    if (location_is(opt->loc, "+")) {
      result = value_add(left, ctx, right);
    } else if (location_is(opt->loc, "-")) {
      result = value_sub(left, ctx, right);
    } else if (location_is(opt->loc, "*")) {
      result = value_mul(left, ctx, right);
    } else if (location_is(opt->loc, "/")) {
      result = value_div(left, ctx, right);
    } else if (location_is(opt->loc, "%")) {
      result = value_mod(left, ctx, right);
    } else if (location_is(opt->loc, "&")) {
      result = value_and(left, ctx, right);
    } else if (location_is(opt->loc, "|")) {
      result = value_or(left, ctx, right);
    } else if (location_is(opt->loc, "^")) {
      result = value_xor(left, ctx, right);
    } else if (location_is(opt->loc, ">>")) {
      result = value_shr(left, ctx, right);
    } else if (location_is(opt->loc, "<<")) {
      result = value_shl(left, ctx, right);
    } else if (location_is(opt->loc, "&&")) {
      result = value_logical_and(left, ctx, right);
    } else if (location_is(opt->loc, "||")) {
      result = value_logical_or(left, ctx, right);
    } else if (location_is(opt->loc, "==")) {
      result = value_eq(left, ctx, right);
    } else if (location_is(opt->loc, "!=")) {
      result = value_ne(left, ctx, right);
    } else if (location_is(opt->loc, ">")) {
      result = value_gt(left, ctx, right);
    } else if (location_is(opt->loc, "<")) {
      result = value_lt(left, ctx, right);
    } else if (location_is(opt->loc, ">=")) {
      result = value_ge(left, ctx, right);
    } else if (location_is(opt->loc, "<=")) {
      result = value_le(left, ctx, right);
    } else {
      result = create_error(ctx, "unsupport binary operator");
    }
  } else if (right_node) {
    value_t right = resolve_expression(ctx, right_node);
    if (value_is_error(right)) {
      return right;
    }
    if (value_is_interrupt(right)) {
      return right;
    }
    if (value_is_writer(right)) {
      ast_remove_child(node, "right");
      right_node = create_ast_value_node(allocator, right);
      ast_add_child(allocator, node, "right", right_node);
    }
    if (location_is(opt->loc, "~")) {
      result = value_bitwise_not(right, ctx);
    } else if (location_is(opt->loc, "!")) {
      result = value_logical_not(right, ctx);
    } else if (location_is(opt->loc, "+")) {
      result = value_plus(right, ctx);
    } else if (location_is(opt->loc, "-")) {
      result = value_neg(right, ctx);
    } else if (location_is(opt->loc, "&")) {
      result = value_addr_of(right, ctx);
    } else if (location_is(opt->loc, "*")) {
      result = value_deref(right, ctx);
    } else if (location_is(opt->loc, "typeof")) {
      type_t type = value_get_type(right);
      result = create_type_value(ctx, type, false, NULL);
    } else {
      result = create_error(ctx, "unsupport binary operator");
    }
  } else {
    return create_comptime_error(ctx, node, "invalid expression");
  }
  if (value_is_error(result)) {
    return convert_comptime_error(ctx, node, result);
  }
  return result;
}