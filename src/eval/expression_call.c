#include "eval/expression_call.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/value.h"
#include "eval/expression.h"
value_t eval_expression_call(context_t ctx, ast_node_t node) {
  ast_node_t callee = ast_get_child(node, "callee");
  ast_node_t arguments = ast_get_child(node, "arguments");
  size_t argc = ast_get_length(arguments);
  value_t argv[argc];
  for (size_t idx = 0; idx < argc; idx++) {
    ast_node_t item = ast_get_item(arguments, idx);
    value_t arg = eval_expression(ctx, item);
    if (value_is_error(arg)) {
      return arg;
    }
    if (value_is_interrupt(arg)) {
      return arg;
    }
    if (!value_is_comptime(arg)) {
      return create_compile_error(ctx, item, "expression is not comptime");
    }
    argv[idx] = arg;
  }
  if (callee->type == NODE_TYPE_EXPRESSION_MEMBER) {
    ast_node_t obj_node = ast_get_child(callee, "host");
    ast_node_t field_node = ast_get_child(callee, "field");
    value_t obj = eval_expression(ctx, obj_node);
    if (value_is_error(obj)) {
      return obj;
    }
    if (value_is_interrupt(obj)) {
      return obj;
    }
    if (!value_is_comptime(obj)) {
      return create_compile_error(ctx, obj_node, "expression is not comptime");
    }
    allocator_t allocator = context_get_allocator(ctx);
    char *field = location_get(field_node->loc, allocator);
    value_t res = value_member_call(obj, ctx, field, argc, argv);
    allocator_free(allocator, field);
    if (value_is_error(res)) {
      return convert_compile_error(ctx, node, res);
    }
    return res;
  } else {
    value_t func = eval_expression(ctx, callee);
    if (value_is_error(func)) {
      return func;
    }
    if (value_is_interrupt(func)) {
      return func;
    }
    if (!value_is_comptime(func)) {
      return create_compile_error(ctx, callee, "expression is not comptime");
    }
    value_t res = value_call(func, ctx, argc, argv);
    if (value_is_error(res)) {
      return convert_compile_error(ctx, node, res);
    }
    return res;
  }
}