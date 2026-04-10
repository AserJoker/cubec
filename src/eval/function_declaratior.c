#include "eval/function_declaratior.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/position.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/function.h"
#include "engine/type.h"
#include "engine/value.h"
#include "eval/function_body.h"
#include "eval/type.h"
#include <stdbool.h>
cubec_value_t cubec_eval_function_declaratior(cubec_context_t ctx,
                                              cubec_ast_node_t node) {
  cubec_ast_node_t identifier = cubec_ast_get_child(node, "identifier");
  cubec_ast_node_t type_node = cubec_ast_get_child(node, "type");
  cubec_ast_node_t kind = cubec_ast_get_child(node, "kind");
  cubec_ast_node_t arguments = cubec_ast_get_child(node, "arguments");
  cubec_ast_node_t body = cubec_ast_get_child(node, "body");
  size_t argc = cubec_ast_get_length(arguments);
  cubec_type_t arg_types[argc];
  bool variadic = false;
  for (size_t idx = 0; idx < cubec_ast_get_length(arguments); idx++) {
    cubec_ast_node_t arg_node = cubec_ast_get_item(arguments, idx);
    cubec_ast_node_t type_node = cubec_ast_get_child(arg_node, "type");
    cubec_value_t vtype = cubec_eval_type(ctx, type_node);
    if (cubec_value_is_error(vtype)) {
      return vtype;
    }
    cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
    arg_types[idx] = type;
    if (arg_node->type == CUBEC_NODE_TYPE_FUNCTION_ARGUMENT_REST) {
      variadic = true;
    }
  }
  cubec_value_t vtype = cubec_eval_type(ctx, type_node);
  if (cubec_value_is_error(vtype)) {
    return vtype;
  }
  cubec_type_t return_type = *(cubec_type_t *)cubec_value_get_data(vtype);
  cubec_value_t vfunction_type =
      cubec_create_function_type(ctx, return_type, argc, arg_types, variadic);
  cubec_type_t function_type = *(cubec_type_t *)cubec_value_get_data(vtype);
  char *name = NULL;
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  if (identifier) {
    name = cubec_location_get(identifier->loc, allocator);
  }
  cubec_value_t function =
      cubec_create_function(ctx, function_type, node, false, name);
  cubec_allocator_free(allocator, name);
  cubec_value_t old = cubec_context_set_eval_context(ctx, function);
  cubec_context_push_scope(ctx);
  for (size_t idx = 0; idx < cubec_ast_get_length(arguments); idx++) {
    cubec_ast_node_t arg_node = cubec_ast_get_item(arguments, idx);
    cubec_ast_node_t identifier = cubec_ast_get_child(arg_node, "identifier");
    cubec_ast_node_t type = cubec_ast_get_child(arg_node, "type");
    char *name = cubec_location_get(identifier->loc, allocator);
    cubec_value_t arg =
        cubec_context_create_value(ctx, arg_types[idx], true, NULL, name);
    cubec_allocator_free(allocator, name);
    if (cubec_value_is_error(arg)) {
      return cubec_convert_compile_error(ctx, arg_node, arg);
    }
  }
  cubec_value_t err = cubec_eval_function_body(ctx, body);
  if (cubec_value_is_error(err)) {
    return err;
  }
  cubec_context_pop_scope(ctx);
  cubec_context_set_eval_context(ctx, old);
  return function;
}