#include "eval/expression_assigment.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/value.h"
#include "eval/expression.h"
#include "eval/literal_identifier.h"
cubec_value_t cubec_eval_expression_assigment(cubec_context_t ctx,
                                              cubec_ast_node_t node) {
  cubec_ast_node_t identifier_node = cubec_ast_get_child(node, "identifier");
  cubec_ast_node_t value_node = cubec_ast_get_child(node, "value");
  cubec_ast_node_t opt = cubec_ast_get_child(node, "opt");
  cubec_value_t value = cubec_eval_expression(ctx, value_node);
  if (cubec_value_is_error(value)) {
    return value;
  }
  cubec_value_t dst = cubec_eval_literal_identifier(ctx, identifier_node);
  if (cubec_value_is_error(dst)) {
    return dst;
  }
  if (cubec_location_is(opt->loc, "+=")) {
    value = cubec_value_add(dst, ctx, value);
  } else if (cubec_location_is(opt->loc, "-=")) {
    value = cubec_value_sub(dst, ctx, value);
  } else if (cubec_location_is(opt->loc, "*=")) {
    value = cubec_value_mul(dst, ctx, value);
  } else if (cubec_location_is(opt->loc, "/=")) {
    value = cubec_value_div(dst, ctx, value);
  } else if (cubec_location_is(opt->loc, "%=")) {
    value = cubec_value_mod(dst, ctx, value);
  } else if (cubec_location_is(opt->loc, "&=")) {
    value = cubec_value_and(dst, ctx, value);
  } else if (cubec_location_is(opt->loc, "|=")) {
    value = cubec_value_or(dst, ctx, value);
  } else if (cubec_location_is(opt->loc, "^=")) {
    value = cubec_value_xor(dst, ctx, value);
  } else if (cubec_location_is(opt->loc, ">>=")) {
    value = cubec_value_shr(dst, ctx, value);
  } else if (cubec_location_is(opt->loc, "<<=")) {
    value = cubec_value_shl(dst, ctx, value);
  } else if (!cubec_location_is(opt->loc, "=")) {
    return cubec_create_compile_error(ctx, node,
                                      "unsupport assigment operator");
  }
  if (cubec_value_is_error(value)) {
    return cubec_convert_compile_error(ctx, value_node, value);
  }
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  cubec_value_t res = cubec_value_assigment(dst, ctx, value);
  if (cubec_value_is_error(res)) {
    return cubec_convert_compile_error(ctx, node, res);
  }
  return res;
}