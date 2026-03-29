#include "eval/statement_expression.h"
#include "ast/node.h"
#include "core/map.h"
#include "engine/type.h"
#include "engine/value.h"
#include "eval/expression.h"
cubec_value_t cubec_eval_statement_expression(cubec_context_t ctx,
                                              cubec_ast_node_t sts,
                                              const char *filename) {
  cubec_ast_node_t expression =
      cubec_map_get(sts->children, "expression", NULL);
  cubec_value_t val = cubec_eval_expression(ctx, expression, filename);
  if (!val) {
    return NULL;
  }
  if (val->type->kind == CUBEC_TYPE_KIND_ERROR) {
    return val;
  }
  return ctx->value_undefined;
}