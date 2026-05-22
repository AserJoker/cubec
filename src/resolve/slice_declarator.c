#include "resolve/slice_declarator.h"
#include "ast/node.h"
#include "engine/slice.h"
#include "engine/type.h"
#include "engine/value.h"
#include "resolve/type.h"
#include <stdbool.h>
value_t resolve_slice_declarator(context_t ctx, ast_node_t node) {
  ast_node_t type = ast_get_child(node, "type");
  value_t vtype = resolve_type(ctx, type);
  if (vtype->type->kind == TYPE_KIND_ERROR) {
    return vtype;
  }
  type_t base_type = *(type_t *)vtype->data;
  type_t slice_type = create_slice_type(ctx, base_type);
  return create_type_value(ctx, slice_type, false, NULL);
}