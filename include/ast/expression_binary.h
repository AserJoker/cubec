#ifndef _H_AST_EXPRESSION_BINARY_
#define _H_AST_EXPRESSION_BINARY_
#include "ast/node.h"
#include "core/allocator.h"
#include "reader/token.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
ast_node_t read_expression_logical_or(allocator_t allocator,
                                                 token_stream_t stream);
ast_node_t read_expression_logical_and(allocator_t allocator,
                                                  token_stream_t stream);
ast_node_t read_expression_bitwise_or(allocator_t allocator,
                                                 token_stream_t stream);
ast_node_t read_expression_bitwise_xor(allocator_t allocator,
                                                  token_stream_t stream);
ast_node_t read_expression_bitwise_and(allocator_t allocator,
                                                  token_stream_t stream);
ast_node_t read_expression_equal(allocator_t allocator,
                                            token_stream_t stream);
ast_node_t read_expression_relation(allocator_t allocator,
                                               token_stream_t stream);
ast_node_t read_expression_bitwise_shift(allocator_t allocator,
                                                    token_stream_t stream);
ast_node_t read_expression_additive(allocator_t allocator,
                                               token_stream_t stream);
ast_node_t read_expression_multiplicative(allocator_t allocator,
                                                     token_stream_t stream);
ast_node_t read_expression_prefix(allocator_t allocator,
                                             token_stream_t stream);
#ifdef __cplusplus
}
#endif
#endif