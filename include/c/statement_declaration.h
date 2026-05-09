#ifndef _H_C_STATEMENT_DECLARATION_
#define _H_C_STATEMENT_DECLARATION_
#include "ast/node.h"
#include "core/stream.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif
void write_c_statement_declaration(context_t ctx, ast_node_t node,
                                   stream_t stream);
#ifdef __cplusplus
}
#endif
#endif