#ifndef _H_CUBEC_EVAL_EXPRESSION_CALL_
#define _H_CUBEC_EVAL_EXPRESSION_CALL_
#include "ast/expression_call.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cpluplus
extern "C" {
#endif
cubec_value_t cubec_eval_expression_call(cubec_context_t ctx,
                                         cubec_ast_expression_call_t expr,
                                         const char *filename);
#ifdef __cpluplus
}
#endif
#endif