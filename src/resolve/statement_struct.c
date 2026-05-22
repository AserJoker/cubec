#include "resolve/statement_struct.h"
#include "ast/node.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/void.h"
#include "resolve/struct_declarator.h"

value_t resolve_statement_struct(context_t ctx, ast_node_t node) {
  ast_node_t stru = ast_get_child(node, "struct");
  ast_node_t pub = ast_get_child(stru, "pub");
  if (ctx->type == CONTEXT_TYPE_FUNCTION && pub) {
    return create_comptime_error(ctx, node_get_location(pub),
                                 "invalid pub declarator");
  }
  value_t val = resolve_struct_declarator(ctx, stru);
  if (val->type->kind == TYPE_KIND_ERROR) {
    if (ctx->comptime) {
      return val;
    } else {
      context_push_error(ctx, val);
    }
  } else {
    type_t t = *(type_t *)val->data;
    val = value_clone(val, ctx->allocator);
    value_t err = NULL;
    if (ctx->type == CONTEXT_TYPE_FUNCTION) {
      err = context_declar(ctx, t->name, val);
    } else {
      if (!t->name) {
        return create_comptime_error(ctx, node_get_location(stru),
                                     "missing struct name");
      }
      err =
          struct_type_add_attribute(ctx, ctx->self, t->name, val, pub != NULL);
    }
    if (err->type->kind == TYPE_KIND_ERROR) {
      return convert_comptime_error(ctx, node_get_location(node), err);
    }
  }
  return create_comptime_void(ctx);
}