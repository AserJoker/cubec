#include "resolve/function_declaration.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/function.h"
#include "engine/scope.h"
#include "engine/type.h"
#include "engine/value.h"
#include "resolve/function_body.h"
#include "resolve/type.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
value_t resolve_function_declarator(context_t ctx, ast_node_t node) {
  ast_node_t type_node = ast_get_child(node, "type");
  ast_node_t arguments_node = ast_get_child(node, "arguments");
  ast_node_t identifier_node = ast_get_child(node, "identifier_node");
  ast_node_t closure = ast_get_child(node, "closure");
  ast_node_t kind = ast_get_child(node, "kind");
  allocator_t allocator = context_get_allocator(ctx);
  if (kind && (location_is(kind->loc, "comptime") ||
               location_is(kind->loc, "template"))) {
    value_t function = NULL;
    if (location_is(kind->loc, "template")) {
      function = create_template_function(ctx, node);
    } else {
      function = create_comptime_function(ctx, node);
    }
    for (size_t idx = 0; idx < ast_get_length(closure); idx++) {
      ast_node_t item = ast_get_item(closure, idx);
      char *name = location_get(item->loc, allocator);
      value_t val = context_load(ctx, name);
      if (value_is_error(val)) {
        allocator_free(allocator, name);
        context_pop_scope(ctx);
        return convert_comptime_error(ctx, item, val);
      }
      val = function_add_closure(ctx, function, name, val);
      if (value_is_error(val)) {
        allocator_free(allocator, name);
        context_pop_scope(ctx);
        return convert_comptime_error(ctx, item, val);
      }
      allocator_free(allocator, name);
    }
    return function;
  }
  bool variadic = false;
  size_t argc = ast_get_length(arguments_node);
  context_push_scope(ctx);
  argument_t argv[argc];
  for (size_t idx = 0; idx < ast_get_length(arguments_node); idx++) {
    ast_node_t argument = ast_get_item(arguments_node, idx);
    if (argument->type == NODE_TYPE_FUNCTION_ARGUMENT_REST) {
      variadic = true;
    }
    ast_node_t type = ast_get_child(argument, "type");
    ast_node_t const_ = ast_get_child(argument, "const");
    ast_node_t identifier = ast_get_child(argument, "identifier");
    if (identifier) {
      argv[idx].mut = const_ == NULL;
      value_t vtype = resolve_type(ctx, type);
      if (value_is_error(vtype)) {
        context_pop_scope(ctx);
        return vtype;
      }
      argv[idx].type = *(type_t *)value_get_data(vtype);
      if (type_get_kind(argv[idx].type) == TYPE_KIND_TYPE) {
        if (!kind || !location_is(kind->loc, "comptime")) {
          context_pop_scope(ctx);
          return create_comptime_error(
              ctx, argument, "type value only declared with comptime");
        }
      }
      char *name = location_get(identifier->loc, allocator);
      if (strcmp(name, "_") != 0) {
        value_t err = context_create_value(ctx, argv[idx].type, NULL,
                                           argv[idx].mut, true, name);
        if (value_is_error(err)) {
          allocator_free(allocator, name);
          context_pop_scope(ctx);
          return convert_comptime_error(ctx, argument, err);
        }
      }
      allocator_free(allocator, name);
    } else {
      argv[idx].mut = true;
      argv[idx].type = NULL;
      break;
    }
  }
  value_t vreturn_type = resolve_type(ctx, type_node);
  if (value_is_error(vreturn_type)) {
    context_pop_scope(ctx);
    return vreturn_type;
  }
  type_t return_type = *(type_t *)value_get_data(vreturn_type);
  type_t function_type =
      create_function_type(ctx, return_type, argc, argv, variadic);
  value_t function = create_function(ctx, function_type, node);
  for (size_t idx = 0; idx < ast_get_length(closure); idx++) {
    ast_node_t item = ast_get_item(closure, idx);
    char *name = location_get(item->loc, allocator);
    value_t val = context_load(ctx, name);
    if (value_is_error(val)) {
      allocator_free(allocator, name);
      context_pop_scope(ctx);
      return convert_comptime_error(ctx, item, val);
    }
    val = function_add_closure(ctx, function, name, val);
    if (value_is_error(val)) {
      allocator_free(allocator, name);
      context_pop_scope(ctx);
      return convert_comptime_error(ctx, item, val);
    }
    allocator_free(allocator, name);
  }
  if (context_get_type(ctx) == CONTEXT_TYPE_FUNCTION) {
    resolve_function_declaration(ctx, function);
  }
  context_pop_scope(ctx);
  return function;
}
value_t resolve_function_declaration(context_t ctx, value_t function) {
  function_declar declar = *(function_declar *)value_get_data(function);
  ast_node_t node = declar->node;
  ast_node_t kind = ast_get_child(node, "kind");
  ast_node_t body = ast_get_child(node, "body");
  if (kind && (location_is(kind->loc, "comptime") ||
               location_is(kind->loc, "template") ||
               location_is(kind->loc, "extern"))) {
    return context_get_undefined(ctx);
  }
  type_t function_type = value_get_type(function);
  array_t arguments_type = function_type_get_arguments(function_type);
  ast_node_t arguments_node = ast_get_child(node, "arguments");
  type_t self = declar->bind;
  type_t global = declar->global;
  type_t current_self = context_set_self(ctx, self);
  value_t current_function = context_set_function(ctx, function);
  context_type_t current_type = context_get_type(ctx);
  context_set_type(ctx, CONTEXT_TYPE_FUNCTION);
  type_t current_global = context_get_global(ctx);
  context_set_global(ctx, global);
  bool current_comptime = context_is_comptime(ctx);
  context_set_comptime(ctx, false);
  allocator_t allocator = context_get_allocator(ctx);
  scope_t current_scope = context_get_scope(ctx);
  scope_t scope = create_scope(allocator, context_get_root_scope(ctx));
  context_set_scope(ctx, scope);
  hash_map_t closure = declar->closure;
  list_node_t it = hash_map_get_first(closure);
  while (it != hash_map_get_end(closure)) {
    const char *name = hash_map_node_get_key(it);
    value_t value = hash_map_node_get_value(it);
    value = value_clone(value, allocator);
    scope_store(scope, allocator, name, value);
    it = hash_map_node_get_next(it);
  }
  context_push_scope(ctx);
  scope = context_get_scope(ctx);
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