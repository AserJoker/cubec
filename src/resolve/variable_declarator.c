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

value_t resolve_variable_declarator(context_t ctx, ast_node_t node) {
  ast_node_t identifier = ast_get_child(node, "identifier");
  ast_node_t initialize = ast_get_child(node, "initialize");
  ast_node_t type_node = ast_get_child(node, "type");
  value_t value = NULL;
  initialize = ast_unwrap_group(initialize);
  allocator_t allocator = context_get_allocator(ctx);
  if (initialize->type == CUBEC_NODE_TYPE_INITIALIZE_LIST) {
    ast_node_t itype = ast_get_child(initialize, "type");
    if (!itype && !type_node) {
      return create_compile_error(ctx, node,
                                  "missing type for initialize list");
    }
    type_node = ast_move_child(node, "type");
    ast_add_child(allocator, initialize, "type", type_node);
    type_node = NULL;
  }
  if (context_is_comptime(ctx)) {
    value = eval_expression(ctx, initialize);
  } else {
    value = resolve_expression(ctx, initialize);
  }
  if (value_is_error(value)) {
    return value;
  }
  type_t type = NULL;
  if (type_node) {
    value_t vtype = resolve_type(ctx, type_node);
    if (value_is_error(vtype)) {
      return vtype;
    }
    type = *(type_t *)value_get_data(vtype);
  } else {
    type = value_get_type(value);
  }
  type_t value_type = value_get_type(value);
  if (!type_is_equal(value_type, type)) {
    value = value_safe_convert(value, ctx, type);
    if (value_is_error(value)) {
      return value;
    }
  }
  void *data = value_get_data(value);
  char *name = location_get(identifier->loc, allocator);
  value = context_create_value(ctx, type, true, data, name);
  allocator_free(allocator, name);
  return value;
}