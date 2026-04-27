#include "resolve/struct_declarator.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/module.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/value.h"
#include "resolve/statement_declaration.h"
#include "resolve/statement_function.h"
#include "resolve/statement_struct.h"
#include "resolve/type.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
value_t resolve_struct_declarator(context_t ctx, ast_node_t node) {
  allocator_t allocator = context_get_allocator(ctx);
  ast_node_t identifier = ast_get_child(node, "identifier");
  ast_node_t fields = ast_get_child(node, "fields");
  ast_node_t methods = ast_get_child(node, "methods");
  ast_node_t attributes = ast_get_child(node, "attributes");
  module_t mod = context_get_module(ctx);
  char *name = NULL;
  if (identifier) {
    name = location_get(identifier->loc, allocator);
  }
  type_t stru = create_struct_type(ctx, name, 4);
  context_store_type(ctx, stru);
  allocator_free(allocator, name);
  context_type_t current_type = context_get_type(ctx);
  value_t current_function = context_get_function(ctx);
  type_t current_self = context_get_self(ctx);
  context_set_type(ctx, CONTEXT_TYPE_STRUCT);
  context_set_function(ctx, NULL);
  context_set_self(ctx, stru);
  for (size_t idx = 0; idx < ast_get_length(fields); idx++) {
    ast_node_t field = ast_get_item(fields, idx);
    if (field->type == NODE_TYPE_STRUCT_FIELD) {
      ast_node_t identifier = ast_get_child(field, "identifier");
      ast_node_t type_node = ast_get_child(field, "type");
      ast_node_t mut_node = ast_get_child(field, "mut");
      ast_node_t pub_node = ast_get_child(field, "pub");
      bool mut = mut_node = NULL;
      bool pub = pub_node != NULL;
      value_t vtype = resolve_type(ctx, type_node);
      if (value_is_error(vtype) || value_is_interrupt(vtype)) {
        return vtype;
      }
      ast_node_bind_value(allocator, type_node, vtype);
      type_t type = *(type_t *)value_get_data(vtype);
      char *field_name = location_get(identifier->loc, allocator);
      struct_type_add_field(stru, allocator, field_name, type, mut, pub);
      allocator_free(allocator, field_name);
    } else if (field->type == NODE_TYPE_STATEMENT_STRUCT) {
      value_t err = resolve_statement_struct(ctx, field);
      if (value_is_error(err) || value_is_interrupt(err)) {
        return err;
      }
    } else if (field->type == NODE_TYPE_STATEMENT_FUNCTION) {
      value_t err = resolve_statement_function(ctx, field);
      if (value_is_error(err) || value_is_interrupt(err)) {
        return err;
      }
    } else if (field->type == NODE_TYPE_STATEMENT_DECLARATION) {
      value_t err = resolve_statement_declaration(ctx, field);
      if (value_is_error(err) || value_is_interrupt(err)) {
        return err;
      }
    }
  }
  context_set_self(ctx, current_self);
  context_set_function(ctx, current_function);
  context_set_type(ctx, current_type);
  return create_type_value(ctx, stru, false, NULL);
}