#ifndef _H_AST_EXPRESSION_
#define _H_AST_EXPRESSION_
#include "ast/node.h"
#include "core/allocator.h"
#include "reader/token.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
ast_node_t read_expression(allocator_t allocator, token_stream_t stream);
ast_node_t read_expression_single(allocator_t allocator, token_stream_t stream);
ast_node_t read_expression_value(allocator_t allocator, token_stream_t stream);
ast_node_t read_expression_atom(allocator_t allocator, token_stream_t stream);

#ifdef __cplusplus
}
#endif
#endif