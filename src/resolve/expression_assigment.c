#include "resolve/expression_assigment.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/type.h"
#include "engine/value.h"
#include "resolve/expression.h"
#include <stdbool.h>

static value_t resolve_assigment_opt(context_t ctx, value_t left, value_t right,
                                     ast_node_t opt) {
  if (node_location_is(opt, "+=")) {
    return value_opt_add(left, ctx, right);
  } else if (node_location_is(opt, "-=")) {
    return value_opt_sub(left, ctx, right);
  } else if (node_location_is(opt, "*=")) {
    return value_opt_mul(left, ctx, right);
  } else if (node_location_is(opt, "/=")) {
    return value_opt_div(left, ctx, right);
  } else if (node_location_is(opt, "%=")) {
    return value_opt_mod(left, ctx, right);
  } else if (node_location_is(opt, "&=")) {
    return value_opt_and(left, ctx, right);
  } else if (node_location_is(opt, "|=")) {
    return value_opt_or(left, ctx, right);
  } else if (node_location_is(opt, "^=")) {
    return value_opt_xor(left, ctx, right);
  } else if (node_location_is(opt, ">>=")) {
    return value_opt_shr(left, ctx, right);
  } else if (node_location_is(opt, "<<=")) {
    return value_opt_shl(left, ctx, right);
  }
  return create_comptime_error(ctx, node_get_location(opt),
                               "unsupport assigment opt");
}

value_t resolve_expression_assigment(context_t ctx, ast_node_t node) {
  ast_node_t left = ast_get_child(node, "left");
  ast_node_t right = ast_get_child(node, "right");
  ast_node_t opt = ast_get_child(node, "opt");
  if (node_location_is(left, "_") && !node_location_is(opt, "=")) {
    return create_comptime_error(ctx, node_get_location(node),
                                 "'_' is not allowed to be assigned");
  }
  value_t rvalue = resolve_expression(ctx, right);
  if (rvalue->type->kind == TYPE_KIND_ERROR) {
    return rvalue;
  }
  if (left->type == NODE_TYPE_LITERAL_IDENTIFIER) {
    if (node_location_is(left, "_")) {
      return rvalue;
    }
    char *name = location_get(node_get_location(left), ctx->allocator);
    value_t lvalue = context_load(ctx, name);
    allocator_free(ctx->allocator, name);
    if (lvalue->type->kind == TYPE_KIND_ERROR) {
      return lvalue;
    }
    if (!node_location_is(opt, "=")) {
      rvalue = resolve_assigment_opt(ctx, lvalue, rvalue, opt);
      if (rvalue->type->kind == TYPE_KIND_ERROR) {
        rvalue = convert_comptime_error(ctx, node_get_location(node), rvalue);
        return rvalue;
      }
    }
    value_t err = value_assigment(lvalue, ctx, rvalue);
    if (err->type->kind == TYPE_KIND_ERROR) {
      return convert_comptime_error(ctx, node_get_location(node), err);
    }
    if (lvalue->comptime && rvalue->comptime) {
      return rvalue;
    } else {
      return context_create_value(ctx, rvalue->type, rvalue->mut, NULL);
    }
  } else if (left->type == NODE_TYPE_EXPRESSION_MEMBER) {
    ast_node_t host = ast_get_child(node, "host");
    ast_node_t field = ast_get_child(node, "field");
    value_t obj = resolve_expression(ctx, host);
    if (obj->type->kind == TYPE_KIND_ERROR) {
      return obj;
    }
    if (node_location_is(field, "*")) {
      value_t lvalue = value_deref(obj, ctx);
      if (lvalue->type->kind == TYPE_KIND_ERROR) {
        return convert_comptime_error(ctx, node_get_location(left), lvalue);
      }
      if (!node_location_is(opt, "=")) {
        rvalue = resolve_assigment_opt(ctx, lvalue, rvalue, opt);
        if (rvalue->type->kind == TYPE_KIND_ERROR) {
          return rvalue;
        }
      }
      value_t err = value_assigment(lvalue, ctx, rvalue);
      if (err->type->kind == TYPE_KIND_ERROR) {
        return convert_comptime_error(ctx, node_get_location(node), err);
      }
      if (lvalue->comptime && rvalue->comptime) {
        return rvalue;
      } else {
        return context_create_value(ctx, rvalue->type, rvalue->mut, NULL);
      }
    } else if (left->type == NODE_TYPE_LITERAL_IDENTIFIER) {
      char *name = location_get(node_get_location(field), ctx->allocator);
      if (!node_location_is(opt, "=")) {
        value_t lvalue = value_get_field(obj, ctx, name);
        if (lvalue->type->kind == TYPE_KIND_ERROR) {
          allocator_free(ctx->allocator, name);
          return convert_comptime_error(ctx, node_get_location(left), lvalue);
        }
        rvalue = resolve_assigment_opt(ctx, lvalue, rvalue, opt);
        if (rvalue->type->kind == TYPE_KIND_ERROR) {
          allocator_free(ctx->allocator, name);
          return rvalue;
        }
      }
      value_t err = value_set_field(obj, ctx, name, rvalue);
      allocator_free(ctx->allocator, name);
      if (err->type->kind == TYPE_KIND_ERROR) {
        return convert_comptime_error(ctx, node_get_location(node), err);
      }
      if (obj->comptime && rvalue->comptime) {
        return rvalue;
      } else {
        return context_create_value(ctx, rvalue->type, rvalue->mut, NULL);
      }
    } else {
      return create_comptime_error(ctx, node_get_location(left),
                                   "expression is not assigment");
    }
  } else if (left->type == NODE_TYPE_EXPRESSION_COMPUTE_MEMBER) {
    ast_node_t host = ast_get_child(left, "host");
    ast_node_t field = ast_get_child(left, "field");
    value_t obj = resolve_expression(ctx, host);
    if (obj->type->kind == TYPE_KIND_ERROR) {
      return obj;
    }
    value_t f = resolve_expression(ctx, field);
    if (f->type->kind == TYPE_KIND_ERROR) {
      return f;
    }
    if (!node_location_is(opt, "=")) {
      value_t lvalue = value_get(obj, ctx, f);
      if (lvalue->type->kind == TYPE_KIND_ERROR) {
        return convert_comptime_error(ctx, node_get_location(left), lvalue);
      }
      rvalue = resolve_assigment_opt(ctx, lvalue, rvalue, opt);
      if (rvalue->type->kind == TYPE_KIND_ERROR) {
        return rvalue;
      }
    }
    value_t err = value_set(obj, ctx, f, rvalue);
    if (err->type->kind == TYPE_KIND_ERROR) {
      return convert_comptime_error(ctx, node_get_location(node), err);
    }
    if (obj->comptime && f->comptime && rvalue->comptime) {
      return rvalue;
    } else {
      return context_create_value(ctx, rvalue->type, rvalue->mut, NULL);
    }
  } else {
    return create_comptime_error(ctx, node_get_location(left),
                                 "expression is not assigment");
  }
  return rvalue;
}