#ifndef _H_LITERAL_IDENTIFIER_
#define _H_LITERAL_IDENTIFIER_
#include "ast/node.h"
#include "core/stream.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif
void write_c_literal_identifier(context_t ctx, ast_node_t node,
                                stream_t stream);
#ifdef __cplusplus
}
#endif
#endif