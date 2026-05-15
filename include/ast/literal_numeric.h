#ifndef _H_AST_LITERAL_NUMERIC_
#define _H_AST_LITERAL_NUMERIC_
#include "ast/node.h"
#include "core/allocator.h"
#include "reader/token.h"
#ifdef __cplusplus
extern "C" {
#endif

ast_node_t read_literal_numeric(allocator_t allocator, token_stream_t stream);
#ifdef __cplusplus
}
#endif
#endif