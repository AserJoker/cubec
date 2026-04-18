#include "eval/expression.h"
#include "ast/node_type.h"
#include "engine/error.h"
#include "engine/value.h"
#include "eval/expression_binary.h"
#include "eval/expression_call.h"
#include "eval/expression_compute_member.h"
#include "eval/expression_group.h"
#include "eval/expression_member.h"
#include "eval/literal_char.h"
#include "eval/literal_identifier.h"
#include "eval/literal_numeric.h"
#include "eval/literal_string.h"
#include "eval/ptr_declarator.h"
value_t eval_expression(context_t ctx, ast_node_t node) {
  if (node->type == NODE_TYPE_LITERAL_IDENTIFIER) {
    value_t val = eval_literal_identifier(ctx, node);
    if (!value_is_comptime(val)) {
      return create_compile_error(ctx, node, "value is not comptime");
    }
    return val;
  } else if (node->type == NODE_TYPE_LITERAL_NUMERIC) {
    return eval_literal_numeric(ctx, node);
  } else if (node->type == NODE_TYPE_LITERAL_STRING) {
    return eval_literal_string(ctx, node);
  } else if (node->type == NODE_TYPE_LITERAL_CHAR) {
    return eval_literal_char(ctx, node);
  } else if (node->type == NODE_TYPE_PTR_DECLARATOR) {
    return eval_ptr_declarator(ctx, node);
  } else if (node->type == NODE_TYPE_EXPRESSION_GROUP) {
    return eval_expression_group(ctx, node);
  } else if (node->type == NODE_TYPE_EXPRESSION_CALL) {
    return eval_expression_call(ctx, node);
  } else if (node->type == NODE_TYPE_EXPRESSION_BINARY) {
    return eval_expression_binary(ctx, node);
  } else if (node->type == NODE_TYPE_EXPRESSION_MEMBER) {
    return eval_expression_member(ctx, node);
  } else if (node->type == NODE_TYPE_EXPRESSION_COMPUTE_MEMBER) {
    return eval_expression_compute_member(ctx, node);
  }
  return create_compile_error(ctx, node, "unsupport expression");
}