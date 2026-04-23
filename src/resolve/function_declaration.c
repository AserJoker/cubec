#include "resolve/function_declaration.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/location.h"
#include "core/position.h"
#include "engine/context.h"
#include "engine/function.h"
#include "engine/scope.h"
#include "engine/type.h"
#include "engine/value.h"
#include "resolve/function_body.h"
#include "resolve/type.h"
#include <stdbool.h>
value_t resolve_function_declarator(context_t ctx, ast_node_t node) {
  ast_node_t type_node = ast_get_child(node, "type");
  ast_node_t arguments_node = ast_get_child(node, "arguments");
  ast_node_t identifier_node = ast_get_child(node, "identifier_node");
  bool variadic = false;
  size_t argc = ast_get_length(arguments_node);
  allocator_t allocator = context_get_allocator(ctx);
  array_initialize_t argv_initialize = {
      .autofree = true,
  };
  argument_t argv[argc];
  for (size_t idx = 0; idx < ast_get_length(arguments_node); idx++) {
    ast_node_t argument = ast_get_item(arguments_node, idx);
    if (argument->type == NODE_TYPE_FUNCTION_ARGUMENT_REST) {
      variadic = true;
    }
    ast_node_t type = ast_move_child(argument, "type");
    ast_node_t const_ = ast_get_child(argument, "const");
    ast_node_t identifier = ast_get_child(argument, "identifier");
    argv[idx].mut = const_ == NULL;
    value_t vtype = resolve_type(ctx, type);
    if (value_is_error(vtype)) {
      return vtype;
    }
    if (value_is_interrupt(vtype)) {
      return vtype;
    }
    argv[idx].type = *(type_t *)value_get_data(vtype);
    allocator_free(allocator, type);
    type = create_ast_value_node(allocator, vtype);
    ast_add_child(allocator, argument, "type", type);
  }
  value_t vreturn_type = resolve_type(ctx, type_node);
  if (value_is_error(vreturn_type)) {
    return vreturn_type;
  }
  if (value_is_interrupt(vreturn_type)) {
    return vreturn_type;
  }
  type_t return_type = *(type_t *)value_get_data(vreturn_type);
  type_t function_type =
      create_function_type(ctx, return_type, argc, argv, variadic);
  char *name = NULL;
  if (identifier_node) {
    name = location_get(identifier_node->loc, allocator);
  }
  value_t function = create_function(ctx, function_type, node);
  allocator_free(allocator, name);
  type_t self = context_get_self(ctx);
  value_t vself = create_type_value(ctx, self, false, true, NULL);
  type_t global = context_get_global(ctx);
  value_t vglobal = create_type_value(ctx, global, false, true, NULL);
  ast_node_t self_node = create_ast_value_node(allocator, vself);
  ast_add_child(allocator, node, "self", self_node);
  ast_node_t global_node = create_ast_value_node(allocator, vglobal);
  ast_add_child(allocator, node, "global", global_node);
  return function;
}
value_t resolve_function_declaration(context_t ctx, value_t function) {
  ast_node_t node = *(ast_node_t *)value_get_data(function);
  ast_node_t kind = ast_get_child(node, "kind");
  ast_node_t body = ast_get_child(node, "body");
  if (kind && location_is(kind->loc, "comptime")) {
    return context_get_undefined(ctx);
  }
  type_t function_type = value_get_type(function);
  array_t arguments_type = function_type_get_arguments(function_type);
  ast_node_t self_node = ast_get_child(node, "self");
  ast_node_t global_node = ast_get_child(node, "global");
  ast_node_t arguments_node = ast_get_child(node, "arguments");
  type_t self = *(type_t *)value_get_data(self_node->value);
  type_t global = *(type_t *)value_get_data(global_node->value);
  type_t current_self = context_set_self(ctx, self);
  value_t current_function = context_set_function(ctx, function);
  context_type_t current_type = context_get_type(ctx);
  context_set_type(ctx, CONTEXT_TYPE_FUNCTION);
  type_t current_global = context_get_global(ctx);
  context_set_global(ctx, global);
  bool current_comptime = context_is_comptime(ctx);
  context_set_comptime(ctx, false);
  allocator_t allocator = context_get_allocator(ctx);
  scope_t scope = create_scope(allocator, context_get_root_scope(ctx));
  scope_t current_scope = context_get_scope(ctx);
  context_set_scope(ctx, scope);
  array_t arguments = function_type_get_arguments(function_type);
  for (size_t idx = 0; idx < array_get_size(arguments); idx++) {
    argument_t *arg = array_get(arguments, idx);
    ast_node_t arg_node = ast_get_item(arguments_node, idx);
    ast_node_t identifier = ast_get_child(arg_node, "identifier");
    char *name = location_get(identifier->loc, allocator);
    context_create_value(ctx, arg->type, NULL, arg->mut, false, name);
    allocator_free(allocator, name);
  }
  context_push_scope(ctx);
  resolve_function_body(ctx, body);
  context_pop_scope(ctx);
  context_set_scope(ctx, current_scope);
  allocator_free(allocator, scope);
  context_set_comptime(ctx, current_comptime);
  context_set_global(ctx, current_global);
  context_set_type(ctx, current_type);
  context_set_function(ctx, current_function);
  context_set_self(ctx, current_self);
  return context_get_undefined(ctx);
}