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
value_t resolve_function_declarator(context_t ctx, ast_node_t node) {
  ast_node_t type_node = ast_get_child(node, "type");
  ast_node_t arguments_node = ast_get_child(node, "arguments");
  ast_node_t identifier_node = ast_get_child(node, "identifier_node");
  ast_node_t closure_node = ast_get_child(node, "closure");
  ast_node_t kind = ast_get_child(node, "kind");
  allocator_t allocator = context_get_allocator(ctx);
  value_t function = NULL;
  array_initialize_t init = {
      .autofree = true,
  };
  array_t closure = create_array(allocator, &init);
  for (size_t idx = 0; idx < ast_get_length(closure_node); idx++) {
    ast_node_t item_node = ast_get_item(closure_node, idx);
    char *name = location_get(item_node->loc, allocator);
    value_t val = context_load(ctx, name);
    if (value_is_error(val)) {
      allocator_free(allocator, name);
      allocator_free(allocator, closure);
      return convert_comptime_error(ctx, item_node, val);
    }
    closure_item_t item = create_closure_item(allocator, name, val);
    array_push(closure, item);
    allocator_free(allocator, name);
  }
  if (kind && location_is(kind->loc, "comptime")) {
    return create_comptime_function(ctx, node, closure);
  } else if (kind && location_is(kind->loc, "template")) {
    return create_template_function(ctx, node, closure);
  } else {
    bool variadic = false;
    size_t argc = ast_get_length(arguments_node);
    array_initialize_t init = {
        .autofree = true,
    };
    array_t argv = create_array(allocator, &init);
    scope_t current_scope = context_get_scope(ctx);
    scope_t scope = create_scope(allocator, context_get_root_scope(ctx));
    context_set_scope(ctx, scope);
    for (size_t idx = 0; idx < array_get_size(closure); idx++) {
      closure_item_t item = array_get(closure, idx);
      value_t value = value_clone(item->value, allocator);
      value_t err = context_declar(ctx, item->name, value);
      if (value_is_error(err)) {
        function = err;
        goto onfinish;
      }
    }
    context_push_scope(ctx);
    for (size_t idx = 0; idx < ast_get_length(arguments_node); idx++) {
      ast_node_t argument = ast_get_item(arguments_node, idx);
      if (argument->type == NODE_TYPE_FUNCTION_ARGUMENT_REST) {
        variadic = true;
      }
      ast_node_t type = ast_get_child(argument, "type");
      ast_node_t mut = ast_get_child(argument, "mut");
      ast_node_t identifier = ast_get_child(argument, "identifier");
      argument_t arg =
          allocator_alloc(allocator, sizeof(struct _argument_t), NULL);
      array_push(argv, arg);
      if (identifier) {
        arg->mut = mut == NULL;
        value_t vtype = resolve_type(ctx, type);
        if (value_is_error(vtype)) {
          function = vtype;
          goto onfinish;
        }
        arg->type = *(type_t *)value_get_data(vtype);
        if (type_get_kind(arg->type) == TYPE_KIND_TYPE) {
          function = create_comptime_error(
              ctx, type, "type value only declar comptime context");
          goto onfinish;
        }
        if (type_get_kind(arg->type) == TYPE_KIND_VOID) {
          function =
              create_comptime_error(ctx, type, "cannot declar void argument");
          goto onfinish;
        }
        if (type_get_kind(arg->type) == TYPE_KIND_COMPTIME_FUNCTION) {
          function = create_comptime_error(
              ctx, type, "cannot declar comptime_func argument");
          goto onfinish;
        }
        if (type_get_kind(arg->type) == TYPE_KIND_TEMPLATE_FUNCTION) {
          function = create_comptime_error(
              ctx, type, "cannot declar template_func argument");
          goto onfinish;
        }
        char *name = location_get(identifier->loc, allocator);
        if (strcmp(name, "_") != 0) {
          value_t err =
              context_create_value(ctx, arg->type, NULL, arg->mut, true, name);
          if (value_is_error(err)) {
            allocator_free(allocator, name);
            function = convert_comptime_error(ctx, argument, err);
            goto onfinish;
          }
        }
        allocator_free(allocator, name);
      } else {
        arg->mut = true;
        arg->type = NULL;
        break;
      }
    }
    value_t vreturn_type = resolve_type(ctx, type_node);
    if (value_is_error(vreturn_type)) {
      function = vreturn_type;
      goto onfinish;
    }
    type_t return_type = *(type_t *)value_get_data(vreturn_type);
    type_t function_type =
        create_function_type(ctx, return_type, argv, variadic);
    argv = NULL;
    function = create_function(ctx, function_type, node, closure);
    closure = NULL;
  onfinish:
    allocator_free(allocator, closure);
    allocator_free(allocator, argv);
    function = value_clone(function, allocator);
    scope_store(scope, allocator, NULL, function);
    context_set_scope(ctx, scope);
    return function;
  }
}
value_t resolve_function_declaration(context_t ctx, value_t function) {
  function_declar_t declar = *(function_declar_t *)value_get_data(function);
  type_t function_type = value_get_type(function);
  array_t arguments_type = function_type_get_arguments(function_type);

  ast_node_t node = declar->node;
  ast_node_t body = ast_get_child(node, "body");
  ast_node_t arguments_node = ast_get_child(node, "arguments");
  ast_node_t closure_node = ast_get_child(node, "closure");

  context_frame_t frame = context_push(ctx, function, CONTEXT_TYPE_FUNCTION,
                                       declar->global, declar->bind);
  bool current_comptime = context_set_comptime(ctx, false);
  allocator_t allocator = context_get_allocator(ctx);
  scope_t current_scope = context_get_scope(ctx);
  scope_t scope = create_scope(allocator, context_get_root_scope(ctx));
  context_set_scope(ctx, scope);
  array_t closure = declar->closure;
  for (size_t idx = 0; idx < array_get_size(closure); idx++) {
    closure_item_t item = array_get(closure, idx);
    value_t value = value_clone(item->value, allocator);
    value_t err = context_declar(ctx, item->name, value);
    if (value_is_error(err)) {
      err = convert_comptime_error(ctx, ast_get_item(closure_node, idx), err);
      context_push_error(ctx, err);
      goto onfinish;
    }
  }
  context_push_scope(ctx);
  scope = context_get_scope(ctx);
  array_t arguments = function_type_get_arguments(function_type);
  for (size_t idx = 0; idx < array_get_size(arguments); idx++) {
    argument_t arg = array_get(arguments, idx);
    ast_node_t arg_node = ast_get_item(arguments_node, idx);
    ast_node_t identifier = ast_get_child(arg_node, "identifier");
    char *name = location_get(identifier->loc, allocator);
    value_t err = NULL;
    if (strcmp(name, "_") != 0) {
      value_t err =
          context_create_value(ctx, arg->type, NULL, arg->mut, false, name);
      if (value_is_error(err)) {
        context_push_error(ctx, err);
        goto onfinish;
      }
    }
    allocator_free(allocator, name);
  }
  context_push_scope(ctx);
  resolve_function_body(ctx, body);
onfinish:
  context_set_scope(ctx, current_scope);
  allocator_free(allocator, scope);
  context_set_comptime(ctx, current_comptime);
  context_pop(ctx, frame);
  return context_get_undefined(ctx);
}