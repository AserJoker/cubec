#include "resolve/ptr_declarator.h"
#include "ast/node.h"
#include "engine/ptr.h"
#include "engine/type.h"
#include "resolve/type.h"
#include <stdbool.h>

value_t resolve_ptr_declarator(context_t ctx, ast_node_t node) {
  ast_node_t kind = ast_get_child(node, "kind");
  ast_node_t type_node = ast_get_child(node, "type");
  ast_node_t decorators = ast_get_child(node, "decorators");
  bool mut = true;
  bool vol = false;
  for (size_t idx = 0; idx < ast_get_length(decorators); idx++) {
    ast_node_t decorator = ast_get_item(decorators, idx);
    if (node_location_is(decorator, "const")) {
      mut = false;
    }
    if (node_location_is(decorator, "volatile")) {
      vol = true;
    }
  }
  value_t type = resolve_type(ctx, type_node);
  if (type->type->kind == TYPE_KIND_ERROR) {
    return type;
  }
  type_t t = *(type_t *)type->data;
  type_t ptr_type = NULL;
  if (node_location_is(kind, "*")) {
    ptr_type = create_ptr_type(ctx, t, mut, vol);
  } else {
    // TODO: [*]
  }
  return create_type_value(ctx, ptr_type, false, NULL);
}