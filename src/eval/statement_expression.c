#include "eval/statement_expression.h"
#include "engine/type.h"
#include "engine/value.h"
#include "eval/expression.h"
cubec_value_t
cubec_eval_statement_expression(cubec_context_t ctx,
                                cubec_ast_statement_expression_t sts,
                                const char *filename) {
  cubec_value_t val = cubec_eval_expression(ctx, sts->expression, filename);
  if (val->type->kind == CUBEC_TYPE_KIND_ERROR) {
    return val;
  }
  return ctx->value_undefined;
}