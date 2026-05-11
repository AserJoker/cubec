#include "resolve/slice_declarator.h"
#include "ast/node.h"
#include "core/location.h"
#include "engine/slice.h"
#include "engine/type.h"
#include "engine/value.h"
#include "resolve/expression.h"
#include <stdbool.h>

value_t resolve_slice_declarator(context_t ctx, ast_node_t node) {
  ast_node_t type_node = ast_get_child(node, "type");
  ast_node_t mut_node = ast_get_child(node, "mut");
  bool mut = !mut_node || !location_is(mut_node->loc, "const");
  value_t vtype = resolve_expression(ctx, type_node);
  if (value_is_error(vtype)) {
    return vtype;
  }
  type_t type = *(type_t *)value_get_data(vtype);
  type_t slice_type = create_slice_type(ctx, type, mut);
  return create_type_value(ctx, slice_type, false, NULL);
}