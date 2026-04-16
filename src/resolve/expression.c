#include "resolve/expression.h"
#include "ast/node_type.h"
#include "engine/error.h"
#include "engine/value.h"
#include "eval/literal_char.h"
#include "eval/literal_identifier.h"
#include "eval/literal_numeric.h"
#include "eval/literal_string.h"
#include "eval/ptr_declarator.h"
#include "resolve/expression_binary.h"
#include "resolve/expression_call.h"
#include "resolve/expression_group.h"
#include "resolve/expression_member.h"
value_t resolve_expression(context_t ctx, ast_node_t node) {
  if (node->type == NODE_TYPE_LITERAL_IDENTIFIER) {
    return eval_literal_identifier(ctx, node);
  } else if (node->type == NODE_TYPE_LITERAL_NUMERIC) {
    return eval_literal_numeric(ctx, node);
  } else if (node->type == NODE_TYPE_LITERAL_STRING) {
    return eval_literal_string(ctx, node);
  } else if (node->type == NODE_TYPE_LITERAL_CHAR) {
    return eval_literal_char(ctx, node);
  } else if (node->type == NODE_TYPE_PTR_DECLARATOR) {
    return eval_ptr_declarator(ctx, node);
  } else if (node->type == NODE_TYPE_EXPRESSION_GROUP) {
    return resolve_expression_group(ctx, node);
  } else if (node->type == NODE_TYPE_EXPRESSION_CALL) {
    return resolve_expression_call(ctx, node);
  } else if (node->type == NODE_TYPE_EXPRESSION_BINARY) {
    return resolve_expression_binary(ctx, node);
  } else if (node->type == NODE_TYPE_EXPRESSION_MEMBER) {
    return resolve_expression_member(ctx, node);
  }
  return create_compile_error(ctx, node, "unsupport expression");
}