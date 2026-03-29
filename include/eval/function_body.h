#ifndef _H_CUBEC_EVAL_FUNCTION_BODY_
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cpluplus
extern "C" {
#endif
cubec_value_t cubec_eval_function_body(cubec_context_t ctx,
                                       cubec_ast_node_t body,
                                       const char *filename);
#ifdef __cpluplus
}
#endif
#endif