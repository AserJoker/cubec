#include "resolve/expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/value.h"
#include "resolve/array_declarator.h"
#include "resolve/expression_binary.h"
#include "resolve/expression_call.h"
#include "resolve/expression_member.h"
#include "resolve/function_declaration.h"
#include "resolve/initialize_list.h"
#include "resolve/literal_identifier.h"
#include "resolve/literal_numeric.h"
#include "resolve/literal_string.h"
#include "resolve/ptr_declarator.h"
#include "resolve/struct_declarator.h"
value_t resolve_expression(context_t ctx, ast_node_t node) {
  ast_node_t _value = ast_get_child(node, "_value");
  if (_value) {
    return _value->value;
  }
  value_t value = NULL;
  if (node->type == NODE_TYPE_LITERAL_IDENTIFIER) {
    value = resolve_literal_identifier(ctx, node);
  } else if (node->type == NODE_TYPE_LITERAL_STRING) {
    value = resolve_literal_string(ctx, node);
  } else if (node->type == NODE_TYPE_LITERAL_NUMERIC) {
    value = resolve_literal_numeric(ctx, node);
  } else if (node->type == NODE_TYPE_PTR_DECLARATOR) {
    value = resolve_ptr_declarator(ctx, node);
  } else if (node->type == NODE_TYPE_FUNCTION_DECLARATOR) {
    value = resolve_function_declarator(ctx, node);
  } else if (node->type == NODE_TYPE_ARRAY_DECLARATOR) {
    value = resolve_array_declarator(ctx, node);
  } else if (node->type == NODE_TYPE_INITIALIZE_LIST) {
    value = resolve_initialize_list(ctx, node);
  } else if (node->type == NODE_TYPE_EXPRESSION_BINARY) {
    value = resolve_expression_binary(ctx, node);
  } else if (node->type == NODE_TYPE_EXPRESSION_CALL) {
    value = resolve_expression_call(ctx, node);
  } else if (node->type == NODE_TYPE_EXPRESSION_MEMBER) {
    value = resolve_expression_member(ctx, node);
  } else if (node->type == NODE_TYPE_STRUCT_DECLARATOR) {
    value = resolve_struct_declarator(ctx, node);
  } else if (node->type == NODE_TYPE_VALUE) {
    value = node->value;
  } else {
    return create_comptime_error(ctx, node, "unsupport expression node");
  }
  return value;
}