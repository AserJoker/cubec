#ifndef _H_CUBEC_EVAL_EXPRESSION_MEMBER_
#define _H_CUBEC_EVAL_EXPRESSION_MEMBER_
#include "ast/expression_member.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cpluplus
extern "C" {
#endif
cubec_value_t cubec_eval_expression_member(cubec_context_t ctx,
                                           cubec_ast_expression_member_t expr,
                                           const char *filename);
#ifdef __cpluplus
}
#endif
#endif