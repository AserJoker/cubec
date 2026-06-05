#include "resolve/statement_function.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/void.h"
#include "resolve/expression.h"

value_t resolve_statement_function(context_t ctx, ast_node_t node) {
  ast_node_t func = ast_get_child(node, "function");
  ast_node_t identifier = ast_get_child(func, "identifier");
  char *name = NULL;
  if (identifier) {
    name = location_get(node_get_location(identifier), ctx->allocator);
  }
  ast_node_t pub = ast_get_child(func, "accessor");
  ast_node_t kind = ast_get_child(node, "kind");
  bool is_comptime = kind && node_location_is(kind, "comptime");
  bool is_pub = pub && node_location_is(pub, "pub");
  if (ctx->type == CONTEXT_TYPE_FUNCTION && pub) {
    return create_comptime_error(ctx, node_get_location(pub),
                                 "invalid pub declarator");
  }
  value_t val = resolve_expression(ctx, func);
  if (val->type->kind == TYPE_KIND_ERROR) {
    if (ctx->comptime) {
      return val;
    } else {
      context_push_error(ctx, val);
    }
    allocator_free(ctx->allocator, name);
  } else {
    func = ast_get_child(node, "function");
    if (func->type == NODE_TYPE_VALUE) {
      val = func->value;
    } else {
      ast_node_t bind = ast_get_child(func, "bind");
      val = bind->value;
    }
    if (!name) {
      value_t err = create_comptime_error(ctx, node_get_location(func),
                                          "missing function name");
      if (ctx->comptime) {
        return err;
      } else {
        context_push_error(ctx, err);
        return create_comptime_void(ctx);
      }
    }
    value_t err = NULL;
    if (ctx->type == CONTEXT_TYPE_FUNCTION) {
      val = value_clone(val, ctx->allocator);
      err = context_declar(ctx, name, val);
    } else {
      func = clone_ast_node(ctx->allocator, func);
      err = struct_type_add_method(ctx, ctx->self, name, func, is_pub);
    }
    allocator_free(ctx->allocator, name);
    if (err->type->kind == TYPE_KIND_ERROR) {
      err = convert_comptime_error(ctx, node_get_location(node), err);
      if (ctx->comptime) {
        return err;
      } else {
        context_push_error(ctx, err);
        return create_comptime_void(ctx);
      }
    }
  }
  return create_comptime_void(ctx);
}