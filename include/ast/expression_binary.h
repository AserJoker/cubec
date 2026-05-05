#ifndef _H_AST_EXPRESSION_BINARY_
#define _H_AST_EXPRESSION_BINARY_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/position.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
ast_node_t read_ast_expression_binary_logical_or(allocator_t allocator,
                                                 position_t *position,
                                                 const char *end,
                                                 const char *filename);
ast_node_t read_ast_expression_binary_logical_and(allocator_t allocator,
                                                  position_t *position,
                                                  const char *end,
                                                  const char *filename);
ast_node_t read_ast_expression_binary_bitwise_or(allocator_t allocator,
                                                 position_t *position,
                                                 const char *end,
                                                 const char *filename);
ast_node_t read_ast_expression_binary_bitwise_xor(allocator_t allocator,
                                                  position_t *position,
                                                  const char *end,
                                                  const char *filename);
ast_node_t read_ast_expression_binary_bitwise_and(allocator_t allocator,
                                                  position_t *position,
                                                  const char *end,
                                                  const char *filename);
ast_node_t read_ast_expression_binary_equal(allocator_t allocator,
                                            position_t *position,
                                            const char *end,
                                            const char *filename);
ast_node_t read_ast_expression_binary_relation(allocator_t allocator,
                                               position_t *position,
                                               const char *end,
                                               const char *filename);
ast_node_t read_ast_expression_binary_bitwise_shift(allocator_t allocator,
                                                    position_t *position,
                                                    const char *end,
                                                    const char *filename);
ast_node_t read_ast_expression_binary_additive(allocator_t allocator,
                                               position_t *position,
                                               const char *end,
                                               const char *filename);
ast_node_t read_ast_expression_binary_multiplicative(allocator_t allocator,
                                                     position_t *position,
                                                     const char *end,
                                                     const char *filename);
ast_node_t read_ast_expression_binary_prefix(allocator_t allocator,
                                             position_t *position,
                                             const char *end,
                                             const char *filename);
#ifdef __cplusplus
}
#endif
#endif