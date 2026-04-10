#include "eval/expression_condition.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/position.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/type.h"
#include "engine/value.h"
#include "eval/expression.h"
#include <stdbool.h>

cubec_value_t cubec_eval_expression_condition(cubec_context_t ctx,
                                              cubec_ast_node_t node) {
  cubec_ast_node_t condition_node = cubec_ast_get_child(node, "condition");
  cubec_ast_node_t consequent_node = cubec_ast_get_child(node, "consequent");
  cubec_ast_node_t alternate_node = cubec_ast_get_child(node, "alternate");
  cubec_value_t condition = cubec_eval_expression(ctx, condition_node);
  if (cubec_value_is_error(condition)) {
    return condition;
  }
  if (!cubec_value_type_is(condition, CUBEC_VALUE_TYPE_BOOL)) {
    cubec_value_t vtype = cubec_context_load(ctx, "bool");
    cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
    condition = cubec_value_safe_convert(condition, ctx, type);
    if (cubec_value_is_error(condition)) {
      return cubec_convert_compile_error(ctx, condition_node, condition);
    }
  }
  cubec_value_t consequent = cubec_eval_expression(ctx, consequent_node);
  if (cubec_value_is_error(consequent)) {
    return consequent;
  }
  cubec_type_t consequent_type = cubec_value_get_type(consequent);
  cubec_value_t alternate = cubec_eval_expression(ctx, alternate_node);
  if (cubec_value_is_error(alternate)) {
    return alternate;
  }
  cubec_type_t alternate_type = cubec_value_get_type(alternate);
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  if (!cubec_type_is_equal(consequent_type, alternate_type)) {
    char *ltype_str = cubec_type_to_string(consequent_type, allocator);
    char *rtype_str = cubec_type_to_string(alternate_type, allocator);
    cubec_value_t err = cubec_create_compile_error(
        ctx, node, "invalid operand types for binary expression: '%s' and '%s'",
        ltype_str, rtype_str);
    cubec_allocator_free(allocator, ltype_str);
    cubec_allocator_free(allocator, rtype_str);
    return err;
  }
  return cubec_context_create_value(ctx, consequent_type, false, NULL, NULL);
}