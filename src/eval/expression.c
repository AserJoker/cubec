#include "eval/expression.h"
#include "ast/node_type.h"
#include "eval/expression_binary.h"
#include "eval/expression_call.h"
#include "eval/expression_member.h"
#include "eval/literal_identifier.h"
#include "eval/literal_string.h"
#include "eval/ptr_declarator.h"
cubec_value_t cubec_eval_expression(cubec_context_t ctx, cubec_ast_node_t expr,
                                    const char *filename) {
  if (expr->type == CUBEC_NODE_TYPE_EXPRESSION_BINARY) {
    return cubec_eval_expression_binary(ctx, expr, filename);
  } else if (expr->type == CUBEC_NODE_TYPE_LITERAL_IDENTIFIER) {
    return cubec_eval_literal_identifier(ctx, expr, filename);
  } else if (expr->type == CUBEC_NODE_TYPE_LITERAL_STRING) {
    return cubec_eval_literal_string(ctx, expr, filename);
  } else if (expr->type == CUBEC_NODE_TYPE_PTR_DECLARATOR) {
    return cubec_eval_ptr_declarator(ctx, expr, filename);
  } else if (expr->type == CUBEC_NODE_TYPE_EXPRESSION_MEMBER) {
    return cubec_eval_expression_member(ctx, expr, filename);
  } else if (expr->type == CUBEC_NODE_TYPE_EXPRESSION_CALL) {
    return cubec_eval_expression_call(ctx, expr, filename);
  } else {
    return cubec_context_create_compile_error(ctx, expr, filename,
                                              "Unknown expression type");
  }
}