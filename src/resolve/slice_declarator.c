#include "resolve/slice_declarator.h"
#include "ast/node.h"
#include "engine/slice.h"
#include "engine/type.h"
#include "engine/value.h"
#include "resolve/expression.h"
#include <stdbool.h>

value_t resolve_slice_declarator(context_t ctx, ast_node_t node) {
  ast_node_t type_node = ast_get_child(node, "type");
  value_t vtype = resolve_expression(ctx, type_node);
  if (value_is_error(vtype)) {
    return vtype;
  }
  type_t type = *(type_t *)value_get_data(vtype);
  type_t slice_type = create_slice_type(ctx, type);
  return create_type_value(ctx, slice_type, false, NULL);
}