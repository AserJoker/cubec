#include "resolve/expression_call.h"
#include "ast/expression_group.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/location.h"
#include "engine/error.h"
#include "engine/function.h"
#include "engine/type.h"
#include "engine/value.h"
#include "resolve/expression.h"

value_t resolve_expression_call(context_t ctx, ast_node_t node) {
  ast_node_t callee = ast_get_child(node, "callee");
  ast_node_t arguments = ast_get_child(node, "arguments");
  array_t args = create_array(ctx->allocator, NULL);
  callee = ast_unwrap_group(callee);
  value_t val = NULL;
  if (callee->type == NODE_TYPE_EXPRESSION_MEMBER) {
    ast_node_t host = ast_get_child(callee, "host");
    ast_node_t field = ast_get_child(callee, "field");
    value_t obj = resolve_expression(ctx, host);
    if (obj->type->kind == TYPE_KIND_ERROR) {
      allocator_free(ctx->allocator, args);
      return obj;
    }
    if (obj->type->kind == TYPE_KIND_STRUCT) {
      obj = value_addr(obj, ctx);
      array_push(args, obj);
    }
    char *f = location_get(node_get_location(field), ctx->allocator);
    val = value_get_field(obj, ctx, f);
    allocator_free(ctx->allocator, f);
  } else {
    val = resolve_expression(ctx, callee);
  }
  if (val->type->kind == TYPE_KIND_ERROR) {
    allocator_free(ctx->allocator, args);
    return val;
  }
  function_declar_t declar = *(function_declar_t *)val->data;
  bool is_comptime = ctx->comptime;
  if (declar && declar->kind == FUNCTION_KIND_COMPTIME) {
    ctx->comptime = true;
  }
  for (size_t idx = 0; idx < ast_get_length(arguments); idx++) {
    ast_node_t arg = ast_get_item(arguments, idx);
    value_t value = resolve_expression(ctx, arg);
    if (value->type->kind == TYPE_KIND_ERROR) {
      allocator_free(ctx->allocator, args);
      ctx->comptime = is_comptime;
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
  ctx->comptime = is_comptime;
  return result;
}