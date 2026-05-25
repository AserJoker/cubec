#include "resolve/expression_call.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/array.h"
#include "engine/error.h"
#include "engine/type.h"
#include "engine/value.h"
#include "resolve/expression.h"

value_t resolve_expression_call(context_t ctx, ast_node_t node) {
  ast_node_t callee = ast_get_child(node, "callee");
  ast_node_t arguments = ast_get_child(node, "arguments");
  value_t val = resolve_expression(ctx, callee);
  if (val->type->kind == TYPE_KIND_ERROR) {
    return val;
  }
  array_t args = create_array(ctx->allocator, NULL);
  for (size_t idx = 0; idx < ast_get_length(arguments); idx++) {
    ast_node_t arg = ast_get_item(arguments, idx);
    value_t value = resolve_expression(ctx, arg);
    if (value->type->kind == TYPE_KIND_ERROR) {
      allocator_free(ctx->allocator, args);
      return value;
    }
    array_push(args, value);
  }
  value_t result =
      value_call(val, ctx, array_get_size(args), array_get_data(args));
  allocator_free(ctx->allocator, args);
  if (result->type->kind == TYPE_KIND_ERROR) {
    result = convert_comptime_error(ctx, node_get_location(node), result);
  }
  return result;
}