#include "eval/expression.h"
#include "ast/node_type.h"
#include "engine/error.h"
#include "eval/literal_identifier.h"
#include "eval/literal_numeric.h"
cubec_value_t cubec_eval_expression(cubec_context_t ctx,
                                    cubec_ast_node_t node) {
  if (node->type == CUBEC_NODE_TYPE_LITERAL_IDENTIFIER) {
    return cubec_eval_literal_identifier(ctx, node);
  } else if (node->type == CUBEC_NODE_TYPE_LITERAL_NUMERIC) {
    return cubec_eval_literal_numeric(ctx, node);
  }
  return cubec_create_compile_error(ctx, node, "unsupport expression");
}