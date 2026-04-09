#include "eval/expression.h"
#include "ast/node_type.h"
#include "engine/error.h"
#include "eval/expression_group.h"
#include "eval/literal_char.h"
#include "eval/literal_identifier.h"
#include "eval/literal_numeric.h"
#include "eval/literal_string.h"

cubec_value_t cubec_eval_expression(cubec_context_t ctx,
                                    cubec_ast_node_t node) {
  if (node->type == CUBEC_NODE_TYPE_LITERAL_STRING) {
    return cubec_eval_literal_string(ctx, node);
  } else if (node->type == CUBEC_NODE_TYPE_LITERAL_IDENTIFIER) {
    return cubec_eval_literal_identifier(ctx, node);
  } else if (node->type == CUBEC_NODE_TYPE_LITERAL_NUMERIC) {
    return cubec_eval_literal_numeric(ctx, node);
  } else if (node->type == CUBEC_NODE_TYPE_LITERAL_CHAR) {
    return cubec_eval_literal_char(ctx, node);
  } else if (node->type == CUBEC_NODE_TYPE_EXPRESSION_GROUP) {
    return cubec_eval_expression_group(ctx, node);
  }
  return cubec_create_compile_error(ctx, node, "unsupport expression");
}