#include "resolve/statement_struct.h"
#include "ast/node.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/void.h"
#include "resolve/expression.h"

value_t resolve_statement_struct(context_t ctx, ast_node_t node) {
  ast_node_t stru = ast_get_child(node, "struct");
  ast_node_t identifier = ast_get_child(stru, "identifier");
  ast_node_t assessor = ast_get_child(stru, "assessor");
  bool is_pub = assessor && node_location_is(assessor, "pub");
  char *name = NULL;
  if (identifier) {
    name = location_get(node_get_location(identifier), ctx->allocator);
  }
  if (ctx->type == CONTEXT_TYPE_FUNCTION && assessor) {
    return create_comptime_error(ctx, node_get_location(assessor),
                                 "invalid pub declarator");
  }
  value_t val = resolve_expression(ctx, stru);
  if (val->type->kind == TYPE_KIND_ERROR) {
    if (ctx->comptime) {
      return val;
    } else {
      context_push_error(ctx, val);
    }
    allocator_free(ctx->allocator, name);
  } else {
    if (!name) {
      value_t err = create_comptime_error(ctx, node_get_location(stru),
                                          "missing struct name");
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
      stru = ast_get_child(node, "struct");
      stru = clone_ast_node(ctx->allocator, stru);
      err = struct_type_add_attribute(ctx, ctx->self, name, is_pub, stru,
                                      val->type, false, true);
    }
    allocator_free(ctx->allocator, name);
    if (err->type->kind == TYPE_KIND_ERROR) {
      value_t err = convert_comptime_error(ctx, node_get_location(node), err);
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