#include "resolve/struct_declarator.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/function.h"
#include "engine/integer.h"
#include "engine/module.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/unsigned.h"
#include "engine/value.h"
#include "resolve/expression.h"
#include "resolve/function_declaration.h"
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
  ast_node_t fields_node = ast_get_child(node, "fields");
  ast_node_t decorators = ast_get_child(node, "decorators");
  ast_node_t packed = ast_get_child(node, "packed");
  ast_node_t aligned = ast_get_child(node, "aligned");
  module_t mod = context_get_module(ctx);
  char *name = NULL;
  if (identifier) {
    name = location_get(identifier->loc, allocator);
  }
  size_t align = 4;
  if (aligned) {
    value_t val = resolve_expression(ctx, aligned);
    if (value_is_error(val)) {
      return val;
    }
    if (!value_is_comptime(val)) {
      return create_comptime_error(ctx, aligned, "value is not comptime");
    }
    type_t type = value_get_type(val);
    if (type_get_kind(type) == TYPE_KIND_INTEGER) {
      int64_t ival = integer_get_value(val);
      if (ival <= 0) {
        return create_comptime_error(ctx, aligned, "invalid align %" PRIdPTR,
                                     ival);
      }
      align = ival;
    } else if (type_get_kind(type) == TYPE_KIND_UNSIGNED) {
      align = unsigned_get_value(val);
    } else {
      return create_comptime_error(ctx, aligned, "invalid align value");
    }
    if (align <= 0) {
      return create_comptime_error(ctx, aligned, "invalid align %" PRIuPTR,
                                   align);
    }
  } else if (packed) {
    align = 1;
  }
  type_t stru = create_struct_type(ctx, name, align);
  if (aligned) {
    struct_type_lock_align(stru);
  }
  if (packed) {
    struct_type_packed(stru);
    struct_type_lock_align(stru);
  }
  context_store_type(ctx, stru);
  allocator_free(allocator, name);
  context_type_t current_type = context_get_type(ctx);
  value_t current_function = context_get_function(ctx);
  type_t current_self = context_get_self(ctx);
  context_set_type(ctx, CONTEXT_TYPE_STRUCT);
  context_set_function(ctx, NULL);
  context_set_self(ctx, stru);
  for (size_t idx = 0; idx < ast_get_length(fields_node); idx++) {
    ast_node_t field = ast_get_item(fields_node, idx);
    if (field->type == NODE_TYPE_STRUCT_FIELD) {
      ast_node_t identifier = ast_get_child(field, "identifier");
      ast_node_t type_node = ast_get_child(field, "type");
      ast_node_t mut_node = ast_get_child(field, "mut");
      ast_node_t pub_node = ast_get_child(field, "pub");
      bool mut = mut_node = NULL;
      bool pub = pub_node != NULL;
      value_t vtype = resolve_type(ctx, type_node);
      if (value_is_error(vtype)) {
        return vtype;
      }
      type_t type = *(type_t *)value_get_data(vtype);
      char *field_name = location_get(identifier->loc, allocator);
      struct_type_add_field(stru, allocator, field_name, type, mut, pub);
      allocator_free(allocator, field_name);
    } else if (field->type == NODE_TYPE_EXPRESSION_SPREAD) {
      ast_node_t expression = ast_get_child(field, "expression");
      value_t vsub = resolve_type(ctx, expression);
      if (value_is_error(vsub)) {
        return vsub;
      }
      type_t sub = *(type_t *)value_get_data(vsub);
      if (type_get_kind(sub) != TYPE_KIND_STRUCT) {
        return create_comptime_error(ctx, expression, "value is not struct");
      }
      if (sub == stru) {
        return create_comptime_error(ctx, expression, "cycle struct");
      }
      array_t fields = struct_type_get_fields(sub);
      array_t attrs = struct_type_get_attributes(sub);
      for (size_t idx = 0; idx < array_get_size(fields); idx++) {
        struct_field_t field = array_get(fields, idx);
        struct_type_remove_field(stru, field->name);
        struct_type_add_field(stru, allocator, field->name, field->type,
                              field->mut, field->pub);
      }
      for (size_t idx = 0; idx < array_get_size(attrs); idx++) {
        struct_attribute_t attr = array_get(attrs, idx);
        struct_type_remove_attribute(stru, attr->name);
        value_t val = attr->value;
        type_t type = value_get_type(val);
        if (type_get_kind(type) == TYPE_KIND_FUNCTION) {
          function_declar_t declar = *(function_declar_t *)value_get_data(val);
          value_t value = resolve_function_declarator(ctx, declar->node);
          if (value_is_error(value)) {
            return value;
          }
          struct_type_add_attribute(stru, allocator, attr->name, value,
                                    attr->pub);
        } else {
          struct_type_add_attribute(stru, allocator, attr->name, attr->value,
                                    attr->pub);
        }
      }
    } else if (field->type == NODE_TYPE_STATEMENT_STRUCT) {
      value_t err = resolve_statement_struct(ctx, field);
      if (value_is_error(err)) {
        return err;
      }
    } else if (field->type == NODE_TYPE_STATEMENT_FUNCTION) {
      value_t err = resolve_statement_function(ctx, field);
      if (value_is_error(err)) {
        return err;
      }
    } else if (field->type == NODE_TYPE_STATEMENT_DECLARATION) {
      value_t err = resolve_statement_declaration(ctx, field);
      if (value_is_error(err)) {
        return err;
      }
    }
  }
  context_set_self(ctx, current_self);
  context_set_function(ctx, current_function);
  context_set_type(ctx, current_type);
  value_t result = create_type_value(ctx, stru, false, NULL);
  for (size_t idx = 0; idx < ast_get_length(decorators); idx++) {
    ast_node_t dec =
        ast_get_item(decorators, ast_get_length(decorators) - 1 - idx);
    ast_node_t expr = ast_get_child(dec, "expression");
    value_t func = resolve_expression(ctx, expr);
    if (value_is_error(func)) {
      return func;
    }
    result = value_call(func, ctx, 1, &result);
    if (value_is_error(result)) {
      return result;
    }
  }
  return result;
}