#ifndef _H_CUBEC_EVAL_EXPRESSION_BINARY_
#define _H_CUBEC_EVAL_EXPRESSION_BINARY_
#include "ast/expression_binary.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cpluplus
extern "C" {
#endif
cubec_value_t cubec_eval_expression_binary(cubec_context_t ctx,
                                           cubec_ast_expression_binary_t expr,
                                           const char *filename);
#ifdef __cpluplus
}
#endif
#endif