#ifndef _H_CUBEC_AST_STATEMENT_EXPRESSION_
#define _H_CUBEC_AST_STATEMENT_EXPRESSION_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/position.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
ast_node_t read_ast_statement_expression(allocator_t allocator,
                                         position_t *position, const char *end,
                                         const char *filename);
#ifdef __cplusplus
}
#endif
#endif