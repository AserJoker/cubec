#include "resolve/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "engine/error.h"
#include "engine/type.h"
#include "engine/value.h"
#include "resolve/array_declarator.h"
#include "resolve/callable_declarator.h"
#include "resolve/expression_binary.h"
#include "resolve/expression_call.h"
#include "resolve/expression_member.h"
#include "resolve/expression_slice.h"
#include "resolve/function_declarator.h"
#include "resolve/literal_identifier.h"
#include "resolve/literal_numeric.h"
#include "resolve/literal_string.h"
#include "resolve/ptr_declarator.h"
#include "resolve/slice_declarator.h"
#include "resolve/struct_declarator.h"

value_t resolve_expression(context_t ctx, ast_node_t node) {
  value_t val = NULL;
  if (node->type == NODE_TYPE_LITERAL_NUMERIC) {
    val = resolve_literal_numeric(ctx, node);
  } else if (node->type == NODE_TYPE_LITERAL_IDENTIFIER) {
    val = resolve_literal_identifier(ctx, node);
  } else if (node->type == NODE_TYPE_LITERAL_STRING) {
    val = resolve_literal_string(ctx, node);
  } else if (node->type == NODE_TYPE_EXPRESSION_BINARY) {
    val = resolve_expression_binary(ctx, node);
  } else if (node->type == NODE_TYPE_PTR_DECLARATOR) {
    val = resolve_ptr_declarator(ctx, node);
  } else if (node->type == NODE_TYPE_ARRAY_DECLARATOR) {
    val = resolve_array_declarator(ctx, node);
  } else if (node->type == NODE_TYPE_SLICE_DECLARATOR) {
    val = resolve_slice_declarator(ctx, node);
  } else if (node->type == NODE_TYPE_STRUCT_DECLARATOR) {
    val = resolve_struct_declarator(ctx, node);
  } else if (node->type == NODE_TYPE_FUNCTION_DECLARATOR) {
    val = resolve_function_declarator(ctx, node);
  } else if (node->type == NODE_TYPE_CALLABLE_DECLARATOR) {
    val = resolve_callable_declarator(ctx, node);
  } else if (node->type == NODE_TYPE_EXPRESSION_MEMBER) {
    val = resolve_expression_member(ctx, node);
  } else if (node->type == NODE_TYPE_EXPRESSION_SLICE) {
    val = resolve_expression_slice(ctx, node);
  } else if (node->type == NODE_TYPE_EXPRESSION_CALL) {
    val = resolve_expression_call(ctx, node);
  } else if (node->type == NODE_TYPE_VALUE) {
    val = node->value;
  } else {
    val = create_comptime_error(ctx, node_get_location(node),
                                "unsupport expression");
  }
  if (val->type->kind == TYPE_KIND_ERROR) {
    return val;
  }
  if (val->comptime && node->type != NODE_TYPE_FUNCTION_DECLARATOR) {
    allocator_free(ctx->allocator, node->data);
    node->value = value_clone(val, ctx->allocator);
    node->type = NODE_TYPE_VALUE;
  }
  return val;
}