#include "resolve/ptr_declarator.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/ptr.h"
#include "engine/type.h"
#include "engine/value.h"
#include "resolve/type.h"
#include <stdbool.h>

value_t resolve_ptr_declarator(context_t ctx, ast_node_t node) {
  allocator_t allocator = context_get_allocator(ctx);
  ast_node_t type = ast_get_child(node, "type");
  ast_node_t kind = ast_get_child(node, "kind");
  ast_node_t decorators = ast_get_child(node, "decorators");
  bool mut = true;
  bool vol = false;
  for (size_t idx = 0; idx < ast_get_length(decorators); idx++) {
    ast_node_t dec = ast_get_item(decorators, idx);
    if (location_is(dec->loc, "const")) {
      mut = false;
    }
    if (location_is(dec->loc, "volatile")) {
      vol = true;
    }
  }
  value_t vtype = resolve_type(ctx, type);
  if (value_is_error(vtype)) {
    return vtype;
  }
  type_t base_type = *(type_t *)value_get_data(vtype);
  if (location_is(kind->loc, "*")) {
    type_t ptr_type = create_ptr_type(ctx, base_type, mut, vol);
    return create_type_value(ctx, ptr_type, false, NULL);
  } else {
    type_t ptr_type = create_parray_type(ctx, base_type, mut, vol);
    return create_type_value(ctx, ptr_type, false, NULL);
  }
}