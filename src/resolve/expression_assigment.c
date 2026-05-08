#include "resolve/expression_assigment.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/value.h"
#include "resolve/expression.h"
value_t resolve_expression_assigment(context_t ctx, ast_node_t node) {
  ast_node_t identifier = ast_get_child(node, "identifier");
  ast_node_t opt = ast_get_child(node, "opt");
  ast_node_t value = ast_get_child(node, "value");
  value_t right = resolve_expression(ctx, value);
  if (value_is_error(right)) {
    return right;
  }
  allocator_t allocator = context_get_allocator(ctx);
  if (location_is(opt->loc, "=")) {
    if (location_is(identifier->loc, "_")) {
      return context_get_undefined(ctx);
    }
    value_t left = resolve_expression(ctx, identifier);
    if (value_is_error(left)) {
      return left;
    }
    value_t err = value_assigment(left, ctx, right);
    if (value_is_error(err)) {
      return convert_comptime_error(ctx, value, err);
    }
  } else {
    value_t left = resolve_expression(ctx, identifier);
    if (value_is_error(left)) {
      return left;
    }
    if (location_is(opt->loc, "+=")) {
      right = value_add(left, ctx, right);
    } else if (location_is(opt->loc, "-=")) {
      right = value_sub(left, ctx, right);
    } else if (location_is(opt->loc, "*=")) {
      right = value_mul(left, ctx, right);
    } else if (location_is(opt->loc, "/=")) {
      right = value_div(left, ctx, right);
    } else if (location_is(opt->loc, "%=")) {
      right = value_mod(left, ctx, right);
    } else if (location_is(opt->loc, "&=")) {
      right = value_and(left, ctx, right);
    } else if (location_is(opt->loc, "|=")) {
      right = value_or(left, ctx, right);
    } else if (location_is(opt->loc, "^=")) {
      right = value_xor(left, ctx, right);
    } else if (location_is(opt->loc, "&&=")) {
      right = value_logical_and(left, ctx, right);
    } else if (location_is(opt->loc, "||=")) {
      right = value_logical_or(left, ctx, right);
    } else if (location_is(opt->loc, "<<=")) {
      right = value_shl(left, ctx, right);
    } else if (location_is(opt->loc, ">>=")) {
      right = value_shr(left, ctx, right);
    }
    value_t err = value_assigment(left, ctx, right);
    if (value_is_error(err)) {
      return convert_comptime_error(ctx, identifier, err);
    }
    return context_get_undefined(ctx);
  }
  return context_get_undefined(ctx);
}