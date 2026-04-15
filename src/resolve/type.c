#include "resolve/type.h"
#include "engine/error.h"
#include "engine/type.h"
#include "engine/value.h"
#include "eval/expression.h"
value_t resolve_type(context_t ctx, ast_node_t node) {
  value_t vtype = eval_expression(ctx, node);
  if (value_is_error(vtype)) {
    return vtype;
  }
  if (!value_type_is(vtype, VALUE_TYPE_TYPE)) {
    return create_compile_error(ctx, node, "value is not type");
  }
  return vtype;
}