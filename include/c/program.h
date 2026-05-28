#ifndef _H_C_PROGRAM_
#define _H_C_PROGRAM_
#include "ast/node.h"
#include "core/stream.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif
void c_program(context_t ctx, ast_node_t node, stream_t stream);
#ifdef __cplusplus
}
#endif
#endif