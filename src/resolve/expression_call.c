#include "resolve/expression_call.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/value.h"
#include "resolve/expression.h"
value_t resolve_expression_call(context_t ctx, ast_node_t node) {
  ast_node_t callee = ast_get_child(node, "callee");
  ast_node_t arguments = ast_get_child(node, "arguments");
  allocator_t allocator = context_get_allocator(ctx);
  ast_node_t resolved_arguments = create_ast_node(allocator, NODE_TYPE_LIST);
  size_t argc = ast_get_length(arguments);
  value_t argv[argc];
  for (size_t idx = 0; idx < argc; idx++) {
    ast_node_t arg = ast_get_item(arguments, idx);
    value_t val = resolve_expression(ctx, arg);
    if (value_is_error(val)) {
      return val;
    }
    if (value_is_interrupt(val)) {
      return val;
    }
    argv[idx] = val;
    arg = create_ast_value_node(allocator, argv[idx]);
    ast_add_item(resolved_arguments, arg);
  }
  ast_remove_child(node, "arguments");
  ast_add_child(allocator, node, "arguments", resolved_arguments);
  if (callee->type == NODE_TYPE_EXPRESSION_MEMBER) {
    ast_node_t host = ast_get_child(callee, "host");
    ast_node_t field = ast_get_child(callee, "field");
    value_t obj = resolve_expression(ctx, host);
    if (value_is_error(obj)) {
      return obj;
    }
    if (value_is_interrupt(obj)) {
      return obj;
    }
    allocator_t allocator = context_get_allocator(ctx);
    char *name = location_get(field->loc, allocator);
    value_t value = value_member_call(obj, ctx, name, argc, argv);
    allocator_free(allocator, name);
    return value;
  } else {
    value_t func = resolve_expression(ctx, callee);
    if (value_is_error(func)) {
      return func;
    }
    if (value_is_interrupt(func)) {
      return func;
    }
    return value_call(func, ctx, argc, argv);
  }
}