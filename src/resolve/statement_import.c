#include "resolve/statement_import.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/void.h"

value_t resolve_statement_import(context_t ctx, ast_node_t node) {
  ast_node_t source = ast_get_child(node, "source");
  location_t src_loc = node_get_location(source);
  char *src = location_get_str(src_loc, ctx->allocator);
  value_t stru = context_load_module(ctx, src);
  allocator_free(ctx->allocator, src);
  if (stru->type->kind == TYPE_KIND_ERROR) {
    value_t err = convert_comptime_error(ctx, node_get_location(node), stru);
    if (ctx->comptime) {
      return err;
    } else {
      context_push_error(ctx, err);
      return create_comptime_void(ctx);
    }
  } else {
    ast_node_t identifier = ast_get_child(node, "identifier");
    location_t id_loc = node_get_location(identifier);
    char *id = location_get(id_loc, ctx->allocator);
    value_t err = NULL;
    if (ctx->type == CONTEXT_TYPE_FUNCTION) {
      stru = value_clone(stru, ctx->allocator);
      err = context_declar(ctx, id, stru);
    } else {
      ast_node_t initialize = create_ast_value(ctx->allocator, stru);
      err = struct_type_add_attribute(ctx, ctx->self, id, false, initialize,
                                      stru->type, false, true);
    }
    allocator_free(ctx->allocator, id);
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