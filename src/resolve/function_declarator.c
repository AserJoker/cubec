#include "resolve/function_declarator.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/function.h"
#include "engine/ptr.h"
#include "engine/value.h"
#include "eval/type.h"
#include "resolve/function_body.h"
#include <stdalign.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

value_t resolve_function_declarator(context_t ctx, ast_node_t node) {

  ast_node_t identifier = ast_get_child(node, "identifier");
  ast_node_t kind = ast_get_child(node, "kind");
  ast_node_t type = ast_get_child(node, "type");
  ast_node_t body = ast_get_child(node, "body");
  ast_node_t arguments = ast_get_child(node, "arguments");
  ast_node_t closure = ast_get_child(node, "closure");
  // TODO: resolve closure lambda
  value_t vreturn_type = eval_type(ctx, type);
  if (value_is_error(vreturn_type)) {
    return vreturn_type;
  }
  type_t return_type = *(type_t *)value_get_data(vreturn_type);
  size_t argc = ast_get_length(arguments);
  type_t argv[argc];
  bool variadic = false;
  for (size_t idx = 0; idx < argc; idx++) {
    ast_node_t item = ast_get_item(arguments, idx);
    ast_node_t type = ast_get_child(item, "type");
    value_t varg_type = eval_type(ctx, type);
    if (value_is_error(varg_type)) {
      return varg_type;
    }
    if (item->type == NODE_TYPE_FUNCTION_ARGUMENT_REST) {
      variadic = true;
    }
    type_t arg_type = *(type_t *)value_get_data(varg_type);
    argv[idx] = arg_type;
  }
  value_t vfunction_type =
      create_function_type(ctx, return_type, argc, argv, variadic);
  type_t function_type = *(type_t *)value_get_data(vfunction_type);
  char *name = NULL;
  allocator_t allocator = context_get_allocator(ctx);
  if (identifier) {
    name = location_get(identifier->loc, allocator);
  }
  value_t function = create_function(ctx, function_type, node, false, name);
  allocator_free(allocator, name);
  context_push_scope(ctx);
  for (size_t idx = 0; idx < argc; idx++) {
    ast_node_t item = ast_get_item(arguments, idx);
    ast_node_t identifier = ast_get_child(item, "identifier");
    char *name = location_get(identifier->loc, allocator);
    if (item->type == NODE_TYPE_FUNCTION_ARGUMENT) {
      value_t err = context_create_value(ctx, argv[idx], true, NULL, name);
      if (value_is_error(err)) {
        return err;
      }
    } else {
      char len_name[strlen(name) + 16];
      sprintf(len_name, "%s_length", name);
      value_t err = context_create_value(ctx, context_load_type(ctx, "u64"),
                                         true, NULL, len_name);
      if (value_is_error(err)) {
        allocator_free(allocator, name);
        return err;
      }
      value_t varr_type = create_ptr_array_type(ctx, argv[idx], false, false);
      type_t arr_type = *(type_t *)value_get_data(varr_type);
      err = context_create_value(ctx, arr_type, true, NULL, name);
      if (value_is_error(err)) {
        allocator_free(allocator, name);
        return err;
      }
    }
    allocator_free(allocator, name);
  }
  if (body && !kind) {
    context_push_static_scope(ctx, function);
    resolve_function_body(ctx, body);
    context_pop_static_scope(ctx);
  }
  context_pop_scope(ctx);
  value_set_comptime(function, true);
  return function;
}