#include "resolve/type.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/type.h"
#include "engine/value.h"
#include "resolve/expression.h"

value_t resolve_type(context_t ctx, ast_node_t node) {
  bool comptime = context_set_comptime(ctx, true);
  value_t value = resolve_expression(ctx, node);
  context_set_comptime(ctx, comptime);
  type_t type = value_get_type(value);
  if (type_get_kind(type) == TYPE_KIND_ERROR) {
    return value;
  }
  if (type_get_kind(type) == TYPE_KIND_INTERRUPT) {
    return value;
  }
  if (type_get_kind(type) != TYPE_KIND_TYPE) {
    return create_compile_error(ctx, node, "expression is not type");
  }
  return value;
}