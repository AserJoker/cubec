#include "resolve/expression.h"
#include "engine/error.h"
#include "eval/literal_char.h"
#include "eval/literal_identifier.h"
#include "eval/literal_numeric.h"
#include "eval/literal_string.h"
#include "resolve/expression_group.h"
value_t resolve_expression(context_t ctx, ast_node_t node) {
  if (node->type == NODE_TYPE_LITERAL_IDENTIFIER) {
    return eval_literal_identifier(ctx, node);
  } else if (node->type == NODE_TYPE_LITERAL_NUMERIC) {
    return eval_literal_numeric(ctx, node);
  } else if (node->type == NODE_TYPE_LITERAL_STRING) {
    return eval_literal_string(ctx, node);
  } else if (node->type == NODE_TYPE_LITERAL_CHAR) {
    return eval_literal_char(ctx, node);
  } else if (node->type == NODE_TYPE_EXPRESSION_GROUP) {
    return resolve_expression_group(ctx, node);
  }
  return create_compile_error(ctx, node, "unsupport expression");
}