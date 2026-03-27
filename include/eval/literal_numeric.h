#ifndef _H_CUBEC_EVAL_LITERAL_NUMERIC_
#define _H_CUBEC_EVAL_LITERAL_NUMERIC_
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t cubec_eval_literal_numeric(cubec_context_t ctx,
                                         cubec_ast_node_t node,
                                         const char *filename);
#ifdef __cplusplus
}
#endif
#endif