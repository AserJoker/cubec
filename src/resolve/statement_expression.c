#include "resolve/statement_expression.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "engine/error.h"
#include "engine/type.h"
#include "engine/void.h"
#include "resolve/expression.h"

value_t resolve_statement_expression(context_t ctx, ast_node_t node) {
  ast_node_t expression = ast_get_child(node, "expression");
  value_t value = resolve_expression(ctx, expression);
  if (value->type->kind == TYPE_KIND_ERROR) {
    return value;
  }
  if (value->type->kind != TYPE_KIND_VOID &&
      expression->type != NODE_TYPE_EXPRESSION_ASSIGMENT) {
    return create_comptime_error(ctx, node_get_location(expression),
                                 "unused expression value");
  }
  return create_comptime_void(ctx);
}