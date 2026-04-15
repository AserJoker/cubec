#ifndef _H_CUBEC_AST_EXPRESSION_GROUP_
#define _H_CUBEC_AST_EXPRESSION_GROUP_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/position.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

ast_node_t read_ast_expression_group(allocator_t allocator,
                                     position_t *position, const char *end,
                                     const char *filename);

ast_node_t ast_unwrap_group(ast_node_t node);

#ifdef __cplusplus
}
#endif
#endif