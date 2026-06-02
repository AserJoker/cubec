#include "resolve/struct_field.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"
#include "engine/error.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/void.h"
#include "resolve/statement_declaration.h"
#include "resolve/statement_function.h"
#include "resolve/statement_struct.h"
#include "resolve/type.h"

value_t resolve_struct_field(context_t ctx, ast_node_t node) {
  if (node->type == NODE_TYPE_STRUCT_FIELD) {
    ast_node_t identifier = ast_get_child(node, "identifier");
    ast_node_t type = ast_get_child(node, "type");
    ast_node_t pub = ast_get_child(node, "pub");
    ast_node_t mut = ast_get_child(node, "mut");
    value_t vtype = resolve_type(ctx, type);
    if (vtype->type->kind == TYPE_KIND_ERROR) {
      return vtype;
    }
    type_t t = *(type_t *)vtype->data;
    if (t->kind == TYPE_KIND_VOID) {
      return create_comptime_error(ctx, node_get_location(type),
                                   "cannot declar %s struct field", t->id);
    }
    char *id = location_get(node_get_location(identifier), ctx->allocator);
    value_t err =
        struct_type_add_field(ctx, ctx->self, id, t, pub != NULL, mut == NULL);
    allocator_free(ctx->allocator, id);
    if (err->type->kind == TYPE_KIND_ERROR) {
      return convert_comptime_error(ctx, node_get_location(node), err);
    }
  } else if (node->type == NODE_TYPE_STATEMENT_STRUCT) {
    value_t err = resolve_statement_struct(ctx, node);
    if (err->type->kind == TYPE_KIND_ERROR) {
      return convert_comptime_error(ctx, node_get_location(node), err);
    }
  } else if (node->type == NODE_TYPE_STATEMENT_FUNCTION) {
    value_t err = resolve_statement_function(ctx, node);
    if (err->type->kind == TYPE_KIND_ERROR) {
      return convert_comptime_error(ctx, node_get_location(node), err);
    }
  } else if (node->type == NODE_TYPE_STATEMENT_DECLARATION) {
    return resolve_statement_declaration(ctx, node);
  } else if (node->type == NODE_TYPE_EXPRESSION_SPREAD) {
  }
  return create_comptime_void(ctx);
}