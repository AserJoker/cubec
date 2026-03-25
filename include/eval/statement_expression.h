#ifndef _H_CUBEC_EVAL_STATEMENT_EXPRESSION_
#define _H_CUBEC_EVAL_STATEMENT_EXPRESSION_
#include "ast/statement_expression.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cpluplus
extern "C" {
#endif
cubec_value_t
cubec_eval_statement_expression(cubec_context_t ctx,
                                cubec_ast_statement_expression_t sts,
                                const char *filename);
#ifdef __cpluplus
}
#endif
#endif