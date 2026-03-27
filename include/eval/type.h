#ifndef _H_CUBEC_EVAL_TYPE_
#define _H_CUBEC_EVAL_TYPE_
#include "ast/type.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cpluplus
extern "C" {
#endif
cubec_value_t cubec_eval_type(cubec_context_t ctx, cubec_ast_node_t type,
                              const char *filename);
#ifdef __cpluplus
}
#endif
#endif