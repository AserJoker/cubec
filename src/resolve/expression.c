#include "resolve/expression.h"
#include "ast/node_type.h"
#include "engine/error.h"
#include "resolve/expression_binary.h"
#include "resolve/literal_identifier.h"
#include "resolve/literal_numeric.h"
#include "resolve/literal_string.h"
value_t resolve_expression(context_t ctx, ast_node_t node) {
  if (node->type == NODE_TYPE_LITERAL_IDENTIFIER) {
    return resolve_literal_identifier(ctx, node);
  } else if (node->type == NODE_TYPE_LITERAL_STRING) {
    return resolve_literal_string(ctx, node);
  } else if (node->type == NODE_TYPE_LITERAL_NUMERIC) {
    return resolve_literal_numeric(ctx, node);
  } else if (node->type == NODE_TYPE_EXPRESSION_BINARY) {
    return resolve_expression_binary(ctx, node);
  }
  return create_comptime_error(ctx, node, "unsupport expression node");
}