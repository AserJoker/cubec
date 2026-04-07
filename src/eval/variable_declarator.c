#include "eval/variable_declarator.h"
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
cubec_value_t cubec_eval_variable_declarator(cubec_context_t ctx,
                                             cubec_ast_node_t node) {
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  cubec_ast_node_t identifier = cubec_ast_get_child(node, "identifier");
  cubec_ast_node_t initialize = cubec_ast_get_child(node, "initialize");
  cubec_ast_node_t type = cubec_ast_get_child(node, "type");
  initialize = cubec_ast_unwrap_group(initialize);
  if (type) {
    type = cubec_ast_unwrap_group(type);
  }
  cubec_type_t value_type = NULL;
  if (type) {
    cubec_value_t vtype = cubec_eval_expression(ctx, type);
    if (cubec_value_is_error(vtype)) {
      return vtype;
    }
    if (!cubec_value_type_is(vtype, CUBEC_VALUE_TYPE_TYPE)) {
      return cubec_create_compile_error(ctx, type, "Value is not a type");
    }
    value_type = *(cubec_type_t *)cubec_value_get_type(vtype);
    if (!value_type) {
      return cubec_create_compile_error(ctx, type,
                                        "Expression is not comptime");
    }
  }
  if (initialize->type == CUBEC_NODE_TYPE_INITIALIZE_LIST) {
    cubec_ast_node_t itype = cubec_ast_get_child(initialize, "type");
    if (!itype && !type) {
      return cubec_create_compile_error(ctx, node,
                                        "Missing type for initialize list");
    }
    if (!type) {
      cubec_value_t vtype = cubec_eval_expression(ctx, itype);
      if (cubec_value_is_error(vtype)) {
        return vtype;
      }
      if (!cubec_value_type_is(vtype, CUBEC_VALUE_TYPE_TYPE)) {
        return cubec_create_compile_error(ctx, type, "Value is not a type");
      }
      value_type = *(cubec_type_t *)cubec_value_get_type(vtype);
      if (!value_type) {
        return cubec_create_compile_error(ctx, type,
                                          "Expression is not comptime");
      }
    }
    if (!itype) {
      itype = cubec_clone_ast_node(allocator, type);
      cubec_ast_add_child(allocator, initialize, "type", itype);
    }
  }
  cubec_value_t value = NULL;
  if (initialize->type == CUBEC_NODE_TYPE_LITERAL_NUMERIC) {
    // TODO:
  } else {
    value = cubec_eval_expression(ctx, initialize);
  }
  if (cubec_value_is_error(value)) {
    return value;
  }
  if (!cubec_type_is_equal(value_type, cubec_value_get_type(value))) {
    char *dst_type = cubec_type_to_string(value_type, allocator);
    char *src_type =
        cubec_type_to_string(cubec_value_get_type(value), allocator);
    cubec_value_t err = cubec_create_compile_error(
        ctx, node, "Cannot convert '%s' to '%s'", src_type, dst_type);
    cubec_allocator_free(allocator, src_type);
    cubec_allocator_free(allocator, dst_type);
    return err;
  }
  char *name = cubec_location_get(identifier->loc, allocator);
  cubec_value_t err = cubec_context_declar(ctx, name, value);
  cubec_allocator_free(allocator, name);
  if (cubec_value_is_error(err)) {
    return cubec_convert_compile_error(ctx, node, err);
  }
  return value;
}