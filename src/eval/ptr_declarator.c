#include "eval/ptr_declarator.h"
#include "ast/node.h"
#include "core/location.h"
#include "engine/ptr.h"
#include "engine/type.h"
#include "engine/value.h"
#include "eval/type.h"

value_t eval_ptr_declarator(context_t ctx, ast_node_t node) {
  ast_node_t type_node = ast_get_child(node, "type");
  ast_node_t kind = ast_get_child(node, "kind");
  ast_node_t decorators = ast_get_child(node, "decorators");
  value_t type = eval_type(ctx, type_node);
  if (value_is_error(type)) {
    return type;
  }
  if (value_is_interrupt(type)) {
    return type;
  }
  type_t base = *(type_t *)value_get_data(type);
  bool mutable = true;
  bool volatile_ = false;
  for (size_t idx = 0; idx < ast_get_length(decorators); idx++) {
    ast_node_t item = ast_get_item(decorators, idx);
    if (location_is(item->loc, "const")) {
      mutable = false;
    }
    if (location_is(item->loc, "volatile")) {
      volatile_ = true;
    }
  }
  if (location_is(kind->loc, "*")) {
    return create_type_value(ctx, type_get_ptr_type(base, ctx), false, NULL);
  } else {
    return create_ptr_array_type(ctx, base, mutable, volatile_);
  }
}