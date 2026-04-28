#include "resolve/callable_declarator.h"
#include "ast/node.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/function.h"
#include "engine/type.h"
#include "engine/value.h"
#include "resolve/type.h"
#include <stdbool.h>
value_t resolve_callable_declarator(context_t ctx, ast_node_t node) {
  ast_node_t type = ast_get_child(node, "type");
  ast_node_t args = ast_get_child(node, "arguments");
  size_t argc = ast_get_length(args);
  argument_t argv[argc];
  bool mutable = false;
  for (size_t idx = 0; idx < ast_get_length(args); idx++) {
    ast_node_t arg_node = ast_get_item(args, idx);
    ast_node_t mut_node = ast_get_child(arg_node, "mut");
    ast_node_t type_node = ast_get_child(arg_node, "type");
    value_t vtype = resolve_type(ctx, type);
    if (value_is_error(vtype) || value_is_interrupt(vtype)) {
      return vtype;
    }
    type_t type = *(type_t *)value_get_data(vtype);
    argv[idx].type = type;
    argv[idx].mut = mut_node == NULL || !location_is(mut_node->loc, "const");
  }
  value_t vres = resolve_type(ctx, type);
  if (value_is_error(vres) || value_is_interrupt(vres)) {
    return vres;
  }
  type_t return_type = *(type_t *)value_get_data(vres);
  type_t t = create_function_type(ctx, return_type, argc, argv, false);
  return create_type_value(ctx, t, false, NULL);
}