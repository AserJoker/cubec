#include "eval/expression.h"
#include "ast/node_type.h"
#include "engine/error.h"
#include "eval/expression_call.h"
#include "eval/expression_group.h"
#include "eval/literal_char.h"
#include "eval/literal_identifier.h"
#include "eval/literal_numeric.h"
#include "eval/literal_string.h"
#include "eval/ptr_declarator.h"
value_t eval_expression(context_t ctx, ast_node_t node) {
  if (node->type == NODE_TYPE_LITERAL_IDENTIFIER) {
    return eval_literal_identifier(ctx, node);
  } else if (node->type == NODE_TYPE_LITERAL_NUMERIC) {
    return eval_literal_numeric(ctx, node);
  } else if (node->type == NODE_TYPE_LITERAL_STRING) {
    return eval_literal_string(ctx, node);
  } else if (node->type == NODE_TYPE_LITERAL_CHAR) {
    return eval_literal_char(ctx, node);
  } else if (node->type == NODE_TYPE_EXPRESSION_GROUP) {
    return eval_expression_group(ctx, node);
  } else if (node->type == NODE_TYPE_EXPRESSION_CALL) {
    return eval_expression_call(ctx, node);
  } else if (node->type == NODE_TYPE_PTR_DECLARATOR) {
    return eval_ptr_declarator(ctx, node);
  }
  return create_compile_error(ctx, node, "unsupport expression");
}