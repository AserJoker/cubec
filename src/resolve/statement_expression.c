#include "resolve/statement_expression.h"
#include "ast/node.h"
#include "engine/error.h"
#include "engine/type.h"
#include "engine/value.h"
#include "resolve/expression.h"
value_t resolve_statement_expression(context_t ctx, ast_node_t node) {
  ast_node_t expression = ast_get_child(node, "expression");
  value_t value = resolve_expression(ctx, expression);
  if (value_is_error(value)) {
    return value;
  }
  type_t type = value_get_type(value);
  if (type_get_kind(type) != TYPE_KIND_VOID) {
    return create_comptime_error(ctx, node, "unused result");
  }
  return value;
}