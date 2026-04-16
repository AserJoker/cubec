#include "eval/expression_member.h"
#include "core/allocator.h"
#include "engine/error.h"
#include "engine/value.h"
#include "eval/expression.h"
value_t eval_expression_member(context_t ctx, ast_node_t node) {
  ast_node_t host = ast_get_child(node, "host");
  ast_node_t field = ast_get_child(node, "field");
  allocator_t allocator = context_get_allocator(ctx);
  value_t obj = eval_expression(ctx, host);
  if (value_is_error(obj)) {
    return obj;
  }
  char *name = location_get(field->loc, allocator);
  value_t val = value_get_field(obj, ctx, name);
  allocator_free(allocator, name);
  if (value_is_error(val)) {
    return convert_compile_error(ctx, node, val);
  }
  return val;
}