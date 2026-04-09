#ifndef _H_CUBEC_EVAL_FUNCTION_BODY_
#define _H_CUBEC_EVAL_FUNCTION_BODY_
#include "ast/node.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t cubec_eval_function_body(cubec_context_t ctx,
                                       cubec_ast_node_t node);
#ifdef __cplusplus
}
#endif
#endif