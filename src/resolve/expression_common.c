#include "ast/node.h"
#include "engine/type.h"
#include "resolve/expression.h"
#include "resolve/expression_comma.h"
value_t resolve_expression_comma(context_t ctx, ast_node_t node) {
  ast_node_t left = ast_get_child(node, "left");
  ast_node_t right = ast_get_child(node, "right");
  value_t value = resolve_expression(ctx, left);
  if (value->type->kind == TYPE_KIND_ERROR) {
    return value;
  }
  value = resolve_expression(ctx, right);
  if (value->type->kind == TYPE_KIND_ERROR) {
    return value;
  }
  return value;
}
