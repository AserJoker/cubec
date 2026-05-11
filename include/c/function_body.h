#ifndef _H_C_FUNCTION_BODY_
#define _H_C_FUNCTION_BODY_
#include "ast/node.h"
#include "core/stream.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif
void write_c_function_body(context_t ctx, ast_node_t node, stream_t stream);
#ifdef __cplusplus
}
#endif
#endif