#ifndef _H_CUBEC_AST_EXPRESSION_
#define _H_CUBEC_AST_EXPRESSION_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/position.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
ast_node_t read_ast_expression(allocator_t allocator, position_t *position,
                               const char *end, const char *filename);

ast_node_t read_ast_expression1(allocator_t allocator, position_t *position,
                                const char *end, const char *filename);
ast_node_t read_ast_expression2(allocator_t allocator, position_t *position,
                                const char *end, const char *filename);
ast_node_t read_ast_expression3(allocator_t allocator, position_t *position,
                                const char *end, const char *filename);
ast_node_t read_ast_expression4(allocator_t allocator, position_t *position,
                                const char *end, const char *filename);
ast_node_t read_ast_expression5(allocator_t allocator, position_t *position,
                                const char *end, const char *filename);
ast_node_t read_ast_expression6(allocator_t allocator, position_t *position,
                                const char *end, const char *filename);
ast_node_t read_ast_expression7(allocator_t allocator, position_t *position,
                                const char *end, const char *filename);
ast_node_t read_ast_expression8(allocator_t allocator, position_t *position,
                                const char *end, const char *filename);
ast_node_t read_ast_expression9(allocator_t allocator, position_t *position,
                                const char *end, const char *filename);
ast_node_t read_ast_expression10(allocator_t allocator, position_t *position,
                                 const char *end, const char *filename);
ast_node_t read_ast_expression11(allocator_t allocator, position_t *position,
                                 const char *end, const char *filename);
ast_node_t read_ast_expression12(allocator_t allocator, position_t *position,
                                 const char *end, const char *filename);
ast_node_t read_ast_expression13(allocator_t allocator, position_t *position,
                                 const char *end, const char *filename);
ast_node_t read_ast_expression14(allocator_t allocator, position_t *position,
                                 const char *end, const char *filename);
ast_node_t read_ast_expression15(allocator_t allocator, position_t *position,
                                 const char *end, const char *filename);
ast_node_t read_ast_expression16(allocator_t allocator, position_t *position,
                                 const char *end, const char *filename);
ast_node_t read_ast_expression17(allocator_t allocator, position_t *position,
                                 const char *end, const char *filename);
ast_node_t read_ast_expression18(allocator_t allocator, position_t *position,
                                 const char *end, const char *filename);
ast_node_t read_ast_expression19(allocator_t allocator, position_t *position,
                                 const char *end, const char *filename);

#ifdef __cplusplus
}
#endif
#endif