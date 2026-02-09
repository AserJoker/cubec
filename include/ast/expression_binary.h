#ifndef _H_CUBEC_NODE_EXPRESSION_BINARY_
#define _H_CUBEC_NODE_EXPRESSION_BINARY_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/position.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_ast_expression_binary_t {
  struct _cubec_ast_node_t super;
  cubec_ast_node_t left;
  cubec_ast_node_t right;
  cubec_ast_node_t opt;
} *cubec_ast_expression_binary_t;
cubec_ast_expression_binary_t
cubec_create_ast_expression_binary(cubec_allocator_t allocator);
cubec_ast_node_t cubec_read_ast_expression_binary_logical_or(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end);
cubec_ast_node_t cubec_read_ast_expression_binary_logical_and(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end);
cubec_ast_node_t cubec_read_ast_expression_binary_bitwise_or(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end);
cubec_ast_node_t cubec_read_ast_expression_binary_bitwise_xor(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end);
cubec_ast_node_t cubec_read_ast_expression_binary_bitwise_and(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end);
cubec_ast_node_t cubec_read_ast_expression_binary_equal(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end);
cubec_ast_node_t cubec_read_ast_expression_binary_relation(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end);
cubec_ast_node_t cubec_read_ast_expression_binary_bitwise_shift(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end);
cubec_ast_node_t cubec_read_ast_expression_binary_additive(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end);
cubec_ast_node_t cubec_read_ast_expression_binary_multiplicative(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end);
cubec_ast_node_t cubec_read_ast_expression_binary_prefix(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end);
cubec_ast_node_t cubec_read_ast_expression_binary_postfix(
    cubec_allocator_t allocator, cubec_position_t *position, const char *end);
#ifdef __cplusplus
}
#endif
#endif