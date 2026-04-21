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
  allocator_t allocator = context_get_allocator(ctx);
  ast_node_t kind = ast_get_child(node, "kind");
  ast_node_t declarations = ast_get_child(node, "declarations");
  ast_node_t type = ast_get_child(node, "type");
  bool mut = !location_is(type->loc, "const");
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
        value_t err = create_compile_error(ctx, declarator,
                                           "missing type for initialize list");
        if (context_is_comptime(ctx)) {
          return err;
        } else {
          fprintf(stderr, "%s\n", error_get_message(err));
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
    type_t value_type = value_get_type(value);
    if (type_get_kind(value_type) == TYPE_KIND_ERROR) {
      if (context_is_comptime(ctx)) {
        return value;
      } else {
        fprintf(stderr, "%s\n", error_get_message(value));
        continue;
      }
    }
    if (type_get_kind(value_type) == TYPE_KIND_INTERRUPT) {
      return value;
    }
    if (context_is_comptime(ctx) && !value_is_comptime(value)) {
      return create_compile_error(ctx, initialize, "value is not comptime");
    }
    if (type) {
      value_t vdst_type = resolve_type(ctx, type);
      type_t type = value_get_type(vdst_type);
      if (type_get_kind(type) == TYPE_KIND_ERROR) {
        if (context_is_comptime(ctx)) {
          return vdst_type;
        } else {
          fprintf(stderr, "%s\n", error_get_message(vdst_type));
          continue;
        }
      }
      if (type_get_kind(type) == TYPE_KIND_INTERRUPT) {
        return vdst_type;
      }
      type_t dst_type = *(type_t *)value_get_data(vdst_type);
      value = value_safe_convert(value, ctx, dst_type);
      if (type_get_kind(type) == TYPE_KIND_ERROR) {
        value_t err = convert_compile_error(ctx, initialize, value);
        if (context_is_comptime(ctx)) {
          return err;
        } else {
          fprintf(stderr, "%s\n", error_get_message(err));
          continue;
        }
      }
      value_type = dst_type;
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
        fprintf(stderr, "%s\n", error_get_message(value));
        continue;
      }
    }
    value_t binding = context_get_binding(ctx);
    type_t binding_type = value_get_type(binding);
    if (type_get_kind(binding_type) == TYPE_KIND_TYPE) {
      type_t module = *(type_t *)value_get_data(binding);
      struct_type_add_attribute(module, allocator, name, value);
    }
    allocator_free(allocator, name);
  }
  context_set_comptime(ctx, comptime);
  return context_get_undefined(ctx);
}