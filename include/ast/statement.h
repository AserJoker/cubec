#ifndef _H_AST_TYPE_STATEMENT_
#define _H_AST_TYPE_STATEMENT_
#include "ast/node.h"
#include "core/allocator.h"
#include "reader/token.h"
#ifdef __cplusplus
extern "C" {
#endif
ast_node_t read_statement(allocator_t allocator, token_stream_t stream);
#ifdef __cplusplus
}
#endif
#endif