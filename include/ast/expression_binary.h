#ifndef _H_CUBEC_AST_EXPRESSION_BINARY_
#define _H_CUBEC_AST_EXPRESSION_BINARY_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/position.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
cubec_ast_node_t cubec_read_ast_expression_binary_logical_or(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end,
    const char *filename);
cubec_ast_node_t cubec_read_ast_expression_binary_logical_and(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end,
    const char *filename);
cubec_ast_node_t cubec_read_ast_expression_binary_bitwise_or(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end,
    const char *filename);
cubec_ast_node_t cubec_read_ast_expression_binary_bitwise_xor(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end,
    const char *filename);
cubec_ast_node_t cubec_read_ast_expression_binary_bitwise_and(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end,
    const char *filename);
cubec_ast_node_t
cubec_read_ast_expression_binary_equal(cubec_allocator_t allocator,
                                       cubec_position_t *position,
                                       const char *end, const char *filename);
cubec_ast_node_t cubec_read_ast_expression_binary_relation(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end,
    const char *filename);
cubec_ast_node_t cubec_read_ast_expression_binary_bitwise_shift(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end,
    const char *filename);
cubec_ast_node_t cubec_read_ast_expression_binary_additive(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end,
    const char *filename);
cubec_ast_node_t cubec_read_ast_expression_binary_multiplicative(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end,
    const char *filename);
cubec_ast_node_t
cubec_read_ast_expression_binary_prefix(cubec_allocator_t allocator,
                                        cubec_position_t *position,
                                        const char *end, const char *filename);
cubec_ast_node_t
cubec_read_ast_expression_binary_postfix(cubec_allocator_t allocator,
                                         cubec_position_t *position,
                                         const char *end, const char *filename);
#ifdef __cplusplus
}
#endif
#endif