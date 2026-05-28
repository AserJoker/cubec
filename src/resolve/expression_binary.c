#include "resolve/expression_binary.h"
#include "ast/node.h"
#include "engine/bool.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/type.h"
#include "engine/unsigned.h"
#include "engine/value.h"
#include "resolve/expression.h"
#include <stdbool.h>

value_t resolve_expression_binary(context_t ctx, ast_node_t node) {
  ast_node_t left = ast_get_child(node, "left");
  ast_node_t right = ast_get_child(node, "right");
  ast_node_t opt = ast_get_child(node, "opt");
  if (left && right) {
    value_t lvalue = resolve_expression(ctx, left);
    if (lvalue->type->kind == TYPE_KIND_ERROR) {
      return lvalue;
    }
    type_t bool_t = context_load_type(ctx, "bool");
    if (node_location_is(opt, "&&")) {
      if (lvalue->comptime) {
        lvalue = value_safe_convert(lvalue, ctx, bool_t);
        if (lvalue->type->kind == TYPE_KIND_ERROR) {
          return convert_comptime_error(ctx, node_get_location(left), lvalue);
        }
        bool flag = *(bool *)lvalue->data;
        if (flag) {
          value_t rvalue = resolve_expression(ctx, right);
          if (rvalue->type->kind == TYPE_KIND_ERROR) {
            return rvalue;
          }
          rvalue = value_safe_convert(rvalue, ctx, bool_t);
          if (rvalue->type->kind == TYPE_KIND_ERROR) {
            return convert_comptime_error(ctx, node_get_location(right),
                                          rvalue);
          }
          return rvalue;
        } else {
          return lvalue;
        }
      }
    }
    if (node_location_is(opt, "||")) {
      if (lvalue->comptime) {
        lvalue = value_safe_convert(lvalue, ctx, bool_t);
        if (lvalue->type->kind == TYPE_KIND_ERROR) {
          return convert_comptime_error(ctx, node_get_location(left), lvalue);
        }
        bool flag = *(bool *)lvalue->data;
        if (!flag) {
          value_t rvalue = resolve_expression(ctx, right);
          if (rvalue->type->kind == TYPE_KIND_ERROR) {
            return rvalue;
          }
          rvalue = value_safe_convert(rvalue, ctx, bool_t);
          if (rvalue->type->kind == TYPE_KIND_ERROR) {
            return convert_comptime_error(ctx, node_get_location(right),
                                          rvalue);
          }
          return rvalue;
        } else {
          return lvalue;
        }
      }
    }
    value_t rvalue = resolve_expression(ctx, right);
    if (rvalue->type->kind == TYPE_KIND_ERROR) {
      return rvalue;
    }
    if (node_location_is(opt, "&&")) {
      return create_bool(ctx, false, NULL);
    } else if (node_location_is(opt, "||")) {
      return create_bool(ctx, false, NULL);
    } else if (node_location_is(opt, "+")) {
      return value_opt_add(lvalue, ctx, rvalue);
    } else if (node_location_is(opt, "-")) {
      return value_opt_sub(lvalue, ctx, rvalue);
    } else if (node_location_is(opt, "*")) {
      return value_opt_mul(lvalue, ctx, rvalue);
    } else if (node_location_is(opt, "/")) {
      return value_opt_div(lvalue, ctx, rvalue);
    } else if (node_location_is(opt, "%")) {
      return value_opt_mod(lvalue, ctx, rvalue);
    } else if (node_location_is(opt, "&")) {
      return value_opt_and(lvalue, ctx, rvalue);
    } else if (node_location_is(opt, "|")) {
      return value_opt_or(lvalue, ctx, rvalue);
    } else if (node_location_is(opt, "^")) {
      return value_opt_xor(lvalue, ctx, rvalue);
    } else if (node_location_is(opt, "<<")) {
      return value_opt_shl(lvalue, ctx, rvalue);
    } else if (node_location_is(opt, ">>")) {
      return value_opt_shr(lvalue, ctx, rvalue);
    } else if (node_location_is(opt, "==")) {
      return value_opt_eq(lvalue, ctx, rvalue);
    } else if (node_location_is(opt, "!=")) {
      return value_opt_ne(lvalue, ctx, rvalue);
    } else if (node_location_is(opt, ">")) {
      return value_opt_gt(lvalue, ctx, rvalue);
    } else if (node_location_is(opt, "<")) {
      return value_opt_lt(lvalue, ctx, rvalue);
    } else if (node_location_is(opt, ">=")) {
      return value_opt_ge(lvalue, ctx, rvalue);
    } else if (node_location_is(opt, "<=")) {
      return value_opt_le(lvalue, ctx, rvalue);
    }
  } else {
    if (node_location_is(opt, "comptime")) {
      bool is_comptime = ctx->comptime;
      ctx->comptime = true;
      value_t rvalue = resolve_expression(ctx, right);
      ctx->comptime = is_comptime;
      return rvalue;
    } else if (node_location_is(opt, "typeof")) {
      bool is_comptime = ctx->comptime;
      ctx->comptime = false;
      value_t rvalue = resolve_expression(ctx, right);
      ctx->comptime = is_comptime;
      if (rvalue->type->kind == TYPE_KIND_ERROR) {
        return rvalue;
      }
      return create_type_value(ctx, rvalue->type, false, NULL);
    } else if (node_location_is(opt, "sizeof")) {
      bool is_comptime = ctx->comptime;
      ctx->comptime = false;
      value_t rvalue = resolve_expression(ctx, right);
      ctx->comptime = is_comptime;
      if (rvalue->type->kind == TYPE_KIND_ERROR) {
        return rvalue;
      }
      type_t type = rvalue->type;
      if (rvalue->type->kind == TYPE_KIND_TYPE) {
        type = *(type_t *)rvalue->data;
      }
      return create_comptime_u64(ctx, type->size, false, NULL);
    } else if (node_location_is(opt, "alignof")) {
      bool is_comptime = ctx->comptime;
      ctx->comptime = false;
      value_t rvalue = resolve_expression(ctx, right);
      ctx->comptime = is_comptime;
      if (rvalue->type->kind == TYPE_KIND_ERROR) {
        return rvalue;
      }
      type_t type = rvalue->type;
      if (rvalue->type->kind == TYPE_KIND_TYPE) {
        type = *(type_t *)rvalue->data;
      }
      return create_comptime_u64(ctx, type->align, false, NULL);
    }
    value_t rvalue = resolve_expression(ctx, right);
    if (rvalue->type->kind == TYPE_KIND_ERROR) {
      return rvalue;
    }
    if (node_location_is(node, "+")) {
      return value_opt_plu(rvalue, ctx);
    } else if (node_location_is(node, "-")) {
      return value_opt_neg(rvalue, ctx);
    } else if (node_location_is(node, "!")) {
      return value_opt_lnot(rvalue, ctx);
    } else if (node_location_is(node, "~")) {
      return value_opt_not(rvalue, ctx);
    }
  }
  return create_comptime_error(ctx, node_get_location(node),
                               "unsupport expression");
}