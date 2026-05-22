#include "resolve/struct_declarator.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "resolve/struct_field.h"
#include <stdbool.h>
#include <stdio.h>

value_t resolve_struct_declarator(context_t ctx, ast_node_t node) {
  ast_node_t pub = ast_get_child(node, "pub");
  ast_node_t identifier = ast_get_child(node, "identifier");
  ast_node_t fields = ast_get_child(node, "fields");
  char *name = NULL;
  if (identifier) {
    name = location_get(node_get_location(identifier), ctx->allocator);
  }
  type_t stru = create_struct_type(ctx, name);
  allocator_free(ctx->allocator, name);
  value_t err = NULL;
  context_type_t current_type = ctx->type;
  ctx->type = CONTEXT_TYPE_STRUCT;
  type_t current_self = ctx->self;
  ctx->self = stru;
  for (size_t idx = 0; idx < ast_get_length(fields); idx++) {
    ast_node_t field = ast_get_item(fields, idx);
    err = resolve_struct_field(ctx, field);
    if (err->type->kind == TYPE_KIND_ERROR) {
      break;
    }
  }
  ctx->self = current_self;
  ctx->type = current_type;
  if (err && err->type->kind == TYPE_KIND_ERROR) {
    return err;
  }
  value_t vstru = create_type_value(ctx, stru, false, NULL);
  return vstru;
}