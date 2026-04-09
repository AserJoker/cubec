#include "eval/variable_declaratior.h"
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

cubec_value_t cubec_eval_variable_declaratior(cubec_context_t ctx,
                                              cubec_ast_node_t node) {
  cubec_ast_node_t identifier = cubec_ast_get_child(node, "identifier");
  cubec_ast_node_t type = cubec_ast_get_child(node, "type");
  cubec_ast_node_t initialize = cubec_ast_get_child(node, "initialize");
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  initialize = cubec_ast_unwrap_group(initialize);
  if (initialize->type == CUBEC_NODE_TYPE_INITIALIZE_LIST) {
    cubec_ast_node_t itype = cubec_ast_get_child(initialize, "type");
    if (!itype) {
      if (!type) {
        return cubec_create_compile_error(ctx, node,
                                          "Missing type for initialize list");
      }
      type = cubec_ast_move_child(node, "type");
      cubec_ast_add_child(allocator, initialize, "type", type);
      type = NULL;
    }
  }
  cubec_type_t value_type = NULL;
  if (type) {
    cubec_value_t vtype = cubec_eval_expression(ctx, type);
    if (cubec_value_is_error(vtype)) {
      return vtype;
    }
    cubec_type_t t = cubec_value_get_type(vtype);
    if (cubec_type_get_kind(t) != CUBEC_VALUE_TYPE_TYPE) {
      return cubec_create_compile_error(ctx, type, "value is not a type");
    }
    if (!cubec_value_get_data(vtype)) {
      return cubec_create_compile_error(ctx, type, "value is not comptime");
    }
    value_type = *(cubec_type_t *)cubec_value_get_data(vtype);
  }
  cubec_value_t value = cubec_eval_expression(ctx, initialize);
  if (cubec_value_is_error(value)) {
    return value;
  }
  if (value_type) {
    cubec_type_t current_type = cubec_value_get_type(value);
    if (!cubec_type_is_equal(value_type, current_type)) {
      value = cubec_value_safe_convert(value, ctx, value_type);
      if (cubec_value_is_error(value)) {
        return cubec_convert_compile_error(ctx, node, value);
      }
    }
  }
  char *c_id = cubec_location_get(identifier->loc, allocator);
  value = cubec_context_declar(ctx, c_id, value);
  cubec_allocator_free(allocator, c_id);
  if (cubec_value_is_error(value)) {
    return cubec_convert_compile_error(ctx, node, value);
  }
  return value;
}