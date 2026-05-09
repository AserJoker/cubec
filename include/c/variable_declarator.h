#ifndef _H_C_VARIABLE_DECLARATOR_
#define _H_C_VARIABLE_DECLARATOR_
#include "ast/node.h"
#include "core/stream.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif
void write_c_declar(context_t ctx, type_t type, ast_node_t identifier,
                    stream_t stream);
#ifdef __cplusplus
}
#endif
#endif