#include "resolve/expression_call.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/value.h"
#include "resolve/expression.h"
#include <string.h>
value_t resolve_expression_call(context_t ctx, ast_node_t node) {
  allocator_t allocator = context_get_allocator(ctx);
  ast_node_t callee = ast_get_child(node, "callee");
  ast_node_t arguments = ast_get_child(node, "arguments");
  size_t argc = ast_get_length(arguments);
  if (callee->type == NODE_TYPE_LITERAL_IDENTIFIER) {
    char *name = location_get(callee->loc, allocator);
    if (context_has_builtin(ctx, name)) {
      ast_node_t resolved = context_eval_builtin(
          ctx, name, argc, array_get_data(arguments->items));
      value_t value = resolve_expression(ctx, resolved);
      value = context_clone_value(ctx, value);
      allocator_free(allocator, resolved);
      allocator_free(allocator, name);
      if (value_is_error(value)) {
        return convert_comptime_error(ctx, node, value);
      }
      if (value_is_interrupt(value)) {
        return value;
      }
      return value;
    }
    allocator_free(allocator, name);
  }
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
    ast_node_bind_value(allocator, arg, val);
  }
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
    ast_node_bind_value(allocator, host, obj);
    allocator_t allocator = context_get_allocator(ctx);
    char *name = location_get(field->loc, allocator);
    value_t value = value_member_call(obj, ctx, name, argc, argv);
    allocator_free(allocator, name);
    if (value_is_error(value)) {
      return convert_comptime_error(ctx, node, value);
    }
    return value;
  } else {
    value_t func = resolve_expression(ctx, callee);
    if (value_is_error(func)) {
      return func;
    }
    if (value_is_interrupt(func)) {
      return func;
    }
    ast_node_bind_value(allocator, callee, func);
    value_t res = value_call(func, ctx, argc, argv);
    if (value_is_error(res)) {
      return convert_comptime_error(ctx, node, res);
    }
    return res;
  }
}