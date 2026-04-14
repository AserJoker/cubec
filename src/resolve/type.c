#include "resolve/type.h"
#include "engine/error.h"
#include "engine/type.h"
#include "engine/value.h"
#include "eval/expression.h"
cubec_value_t cubec_resolve_type(cubec_context_t ctx, cubec_ast_node_t node) {
  cubec_value_t vtype = cubec_eval_expression(ctx, node);
  if (cubec_value_is_error(vtype)) {
    return vtype;
  }
  if (!cubec_value_type_is(vtype, CUBEC_VALUE_TYPE_TYPE)) {
    return cubec_create_compile_error(ctx, node, "value is not type");
  }
  return vtype;
}