#include "resolve/variable_declarator.h"
#include "ast/expression_group.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/type.h"
#include "engine/value.h"
#include "eval/expression.h"
#include "resolve/expression.h"
#include "resolve/type.h"

cubec_value_t cubec_resolve_variable_declarator(cubec_context_t ctx,
                                                cubec_ast_node_t node) {
  cubec_ast_node_t identifier = cubec_ast_get_child(node, "identifier");
  cubec_ast_node_t initialize = cubec_ast_get_child(node, "initialize");
  cubec_ast_node_t type_node = cubec_ast_get_child(node, "type");
  cubec_value_t value = NULL;
  initialize = cubec_ast_unwrap_group(initialize);
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  if (initialize->type == CUBEC_NODE_TYPE_INITIALIZE_LIST) {
    cubec_ast_node_t itype = cubec_ast_get_child(initialize, "type");
    if (!itype && !type_node) {
      return cubec_create_compile_error(ctx, node,
                                        "missing type for initialize list");
    }
    type_node = cubec_ast_move_child(node, "type");
    cubec_ast_add_child(allocator, initialize, "type", type_node);
    type_node = NULL;
  }
  if (cubec_context_is_comptime(ctx)) {
    value = cubec_eval_expression(ctx, initialize);
  } else {
    value = cubec_resolve_expression(ctx, initialize);
  }
  if (cubec_value_is_error(value)) {
    return value;
  }
  cubec_type_t type = NULL;
  if (type_node) {
    cubec_value_t vtype = cubec_resolve_type(ctx, type_node);
    if (cubec_value_is_error(vtype)) {
      return vtype;
    }
    type = *(cubec_type_t *)cubec_value_get_data(vtype);
  } else {
    type = cubec_value_get_type(value);
  }
  cubec_type_t value_type = cubec_value_get_type(value);
  if (!cubec_type_is_equal(value_type, type)) {
    value = cubec_value_safe_convert(value, ctx, type);
    if (cubec_value_is_error(value)) {
      return value;
    }
  }
  void *data = cubec_value_get_data(value);
  char *name = cubec_location_get(identifier->loc, allocator);
  value = cubec_context_create_value(ctx, type, true, data, name);
  cubec_allocator_free(allocator, name);
  return value;
}