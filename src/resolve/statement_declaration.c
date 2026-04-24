#include "resolve/statement_declaration.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/value.h"
#include "resolve/expression.h"
#include "resolve/type.h"
#include <stdbool.h>
#include <stdio.h>
value_t resolve_statement_declaration(context_t ctx, ast_node_t node) {
  value_t result = NULL;
  allocator_t allocator = context_get_allocator(ctx);
  ast_node_t kind = ast_get_child(node, "kind");
  ast_node_t declarations = ast_get_child(node, "declarations");
  ast_node_t declar_type = ast_get_child(node, "type");
  bool mut = !location_is(declar_type->loc, "const");
  bool comptime = false;
  if (kind) {
    comptime = location_is(kind->loc, "comptime");
  }
  comptime |= context_is_comptime(ctx);
  comptime = context_set_comptime(ctx, comptime);
  for (size_t idx = 0; idx < ast_get_length(declarations); idx++) {
    ast_node_t declarator = ast_get_item(declarations, idx);
    ast_node_t identifier = ast_get_child(declarator, "identifier");
    ast_node_t type = ast_get_child(declarator, "type");
    ast_node_t initialize = ast_get_child(declarator, "initialize");
    if (initialize->type == NODE_TYPE_INITIALIZE_LIST) {
      ast_node_t itype = ast_get_child(initialize, "type");
      if (!itype && !type) {
        value_t err = create_comptime_error(ctx, declarator,
                                            "missing type for initialize list");
        if (context_is_comptime(ctx)) {
          return err;
        } else {
          context_push_error(ctx, err);
          continue;
        }
      }
      if (!itype) {
        type = ast_move_child(declarator, "type");
        ast_add_child(allocator, initialize, "type", type);
        type = NULL;
      }
    }
    value_t value = resolve_expression(ctx, initialize);
    if (value_is_error(value)) {
      if (context_is_comptime(ctx)) {
        return value;
      } else {
        context_push_error(ctx, value);
        continue;
      }
    }
    if (value_is_error(value)) {
      return value;
    }
    if (context_is_comptime(ctx) && !value_is_comptime(value)) {
      return create_comptime_error(ctx, initialize, "value is not comptime");
    }
    type_t value_type = value_get_type(value);
    if (type) {
      value_t vdst_type = resolve_type(ctx, type);
      if (value_is_error(vdst_type)) {
        if (context_is_comptime(ctx)) {
          return vdst_type;
        } else {
          fprintf(stderr, "%s\n", error_get_message(vdst_type));
          result = vdst_type;
          continue;
        }
      }
      if (value_is_interrupt(vdst_type)) {
        return vdst_type;
      }
      type_t dst_type = *(type_t *)value_get_data(vdst_type);
      value = value_safe_convert(value, ctx, dst_type);
      if (value_is_error(value)) {
        value_t err = convert_comptime_error(ctx, initialize, value);
        if (context_is_comptime(ctx)) {
          return err;
        } else {
          context_push_error(ctx, err);
          continue;
        }
      }
      value_type = dst_type;
    }
    if (value_is_writer(value)) {
      ast_remove_child(declarator, "initialize");
      initialize = create_ast_value_node(allocator, value);
      ast_add_child(allocator, declarator, "initialize", initialize);
    }
    value_t vtype = create_type_value(ctx, value_type, false, true, NULL);
    if (value_is_writer(vtype)) {
      ast_remove_child(declarator, "type");
      type = create_ast_value_node(allocator, vtype);
      ast_add_child(allocator, declarator, "type", type);
    }
    char *name = location_get(identifier->loc, allocator);
    if (context_is_comptime(ctx)) {
      void *data = value_get_data(value);
      value = context_create_value(ctx, value_type, data, mut, true, name);
    } else {
      value = context_create_value(ctx, value_type, NULL, mut, false, name);
    }
    value_type = value_get_type(value);
    if (type_get_kind(value_type) == TYPE_KIND_ERROR) {
      allocator_free(allocator, name);
      if (context_is_comptime(ctx)) {
        return value;
      } else {
        context_push_error(ctx, value);
        continue;
      }
    }
    if (context_get_type(ctx) == CONTEXT_TYPE_STRUCT) {
      type_t global = context_get_global(ctx);
      struct_type_add_attribute(global, allocator, name, value);
    }
    allocator_free(allocator, name);
  }
  context_set_comptime(ctx, comptime);
  if (result) {
    return create_comptime_error(ctx, node, "declartion statement error");
  }
  if (kind && location_is(kind->loc, "comptime")) {
    node->visible = false;
  }
  return context_get_undefined(ctx);
}