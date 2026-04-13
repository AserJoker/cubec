#include "eval/expression_binary.h"
#include "ast/node.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/numeric.h"
#include "engine/type.h"
#include "engine/value.h"
#include "eval/expression.h"
#include <stdbool.h>
cubec_value_t cubec_eval_expression_binary(cubec_context_t ctx,
                                           cubec_ast_node_t node) {
  cubec_ast_node_t left_node = cubec_ast_get_child(node, "left");
  cubec_ast_node_t right_node = cubec_ast_get_child(node, "right");
  cubec_ast_node_t opt = cubec_ast_get_child(node, "opt");
  cubec_value_t left = NULL;
  if (left_node) {
    left = cubec_eval_expression(ctx, left_node);
    if (cubec_value_is_error(left)) {
      return left;
    }
  }
  cubec_value_t right = NULL;
  if (right_node) {
    right = cubec_eval_expression(ctx, right_node);
    if (cubec_value_is_error(right)) {
      return right;
    }
  }
  cubec_value_t value = NULL;
  if (left && right) {
    if (cubec_location_is(opt->loc, "+")) {
      value = cubec_value_add(left, ctx, right);
    } else if (cubec_location_is(opt->loc, "-")) {
      value = cubec_value_sub(left, ctx, right);
    } else if (cubec_location_is(opt->loc, "*")) {
      value = cubec_value_mul(left, ctx, right);
    } else if (cubec_location_is(opt->loc, "/")) {
      value = cubec_value_div(left, ctx, right);
    } else if (cubec_location_is(opt->loc, "%")) {
      value = cubec_value_mod(left, ctx, right);
    } else if (cubec_location_is(opt->loc, "&")) {
      value = cubec_value_and(left, ctx, right);
    } else if (cubec_location_is(opt->loc, "|")) {
      value = cubec_value_or(left, ctx, right);
    } else if (cubec_location_is(opt->loc, "^")) {
      value = cubec_value_xor(left, ctx, right);
    } else if (cubec_location_is(opt->loc, "<<")) {
      value = cubec_value_shl(left, ctx, right);
    } else if (cubec_location_is(opt->loc, ">>")) {
      value = cubec_value_shr(left, ctx, right);
    } else if (cubec_location_is(opt->loc, "&&")) {
      value = cubec_value_logical_and(left, ctx, right);
    } else if (cubec_location_is(opt->loc, "||")) {
      value = cubec_value_logical_or(left, ctx, right);
    } else {
      return cubec_create_compile_error(ctx, node, "unsupport binary operator");
    }
  }
  if (right) {
    if (cubec_location_is(opt->loc, "!")) {
      value = cubec_value_logical_not(right, ctx);
    } else if (cubec_location_is(opt->loc, "~")) {
      value = cubec_value_bitwise_not(right, ctx);
    } else if (cubec_location_is(opt->loc, "&")) {
      value = cubec_value_ref(right, ctx);
    } else if (cubec_location_is(opt->loc, "*")) {
      value = cubec_value_unref(right, ctx);
    } else if (cubec_location_is(opt->loc, "sizeof")) {
      cubec_type_t type = cubec_value_get_type(right);
      size_t size = cubec_type_get_size(type);
      value = cubec_create_u64(ctx, size, false, NULL);
    } else if (cubec_location_is(opt->loc, "alignof")) {
      cubec_type_t type = cubec_value_get_type(right);
      size_t align = cubec_type_get_align(type);
      value = cubec_create_u64(ctx, align, false, NULL);
    } else if (cubec_location_is(opt->loc, "typeof")) {
      cubec_type_t type = cubec_value_get_type(right);
      value = cubec_create_type_value(ctx, type, false, NULL);
    } else if (cubec_location_is(opt->loc, "+")) {
      value = cubec_value_plus(right, ctx);
    } else if (cubec_location_is(opt->loc, "-")) {
      value = cubec_value_neg(right, ctx);
    } else {
      return cubec_create_compile_error(ctx, node, "unsupport prefix operator");
    }
  }
  if (!value) {
    return cubec_create_compile_error(ctx, node, "unsupport binary operator");
  }
  if (cubec_value_is_error(value)) {
    return cubec_convert_compile_error(ctx, node, value);
  }
  return value;
}