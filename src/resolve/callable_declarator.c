#include "resolve/callable_declarator.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/array.h"
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
  allocator_t allocator = context_get_allocator(ctx);
  array_initialize_t init = {
      .autofree = true,
  };
  array_t argv = create_array(allocator, &init);
  bool variadic = false;
  for (size_t idx = 0; idx < ast_get_length(args); idx++) {
    ast_node_t arg_node = ast_get_item(args, idx);
    ast_node_t mut_node = ast_get_child(arg_node, "mut");
    ast_node_t type_node = ast_get_child(arg_node, "type");
    argument_t arg =
        allocator_alloc(allocator, sizeof(struct _argument_t), NULL);
    arg->mut = true;
    arg->type = NULL;
    array_push(argv, arg);
    if (arg_node->type == NODE_TYPE_FUNCTION_ARGUMENT_REST) {
      variadic = true;
    }
    if (type_node) {
      value_t vtype = resolve_type(ctx, type);
      if (value_is_error(vtype)) {
        allocator_free(allocator, argv);
        return vtype;
      }
      type_t type = *(type_t *)value_get_data(vtype);
      arg->type = type;
      arg->mut = mut_node == NULL;
    }
  }
  value_t vres = resolve_type(ctx, type);
  if (value_is_error(vres)) {
    allocator_free(allocator, argv);
    return vres;
  }
  type_t return_type = *(type_t *)value_get_data(vres);
  type_t t = create_function_type(ctx, return_type, argv, variadic);
  return create_type_value(ctx, t, false, NULL);
}