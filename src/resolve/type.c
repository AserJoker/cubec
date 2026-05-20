#include "resolve/type.h"
#include "ast/node.h"
#include "engine/error.h"
#include "engine/type.h"
#include "resolve/expression.h"

value_t resolve_type(context_t ctx, ast_node_t node) {
  value_t vtype = resolve_expression(ctx, node);
  if (vtype->type->kind == TYPE_KIND_ERROR) {
    return vtype;
  }
  if (vtype->type->kind != TYPE_KIND_TYPE) {
    return create_comptime_error(ctx, node_get_location(node),
                                 "expresion is not type");
  }
  return vtype;
}