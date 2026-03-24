#ifndef _H_CUBEC_RUNTIME_EVAL_
#define _H_CUBEC_RUNTIME_EVAL_
#include "ast/node.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t cubec_runtime_eval(cubec_context_t self, cubec_ast_node_t node);
#ifdef __cplusplus
}
#endif
#endif