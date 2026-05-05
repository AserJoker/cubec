#ifndef _H_AST_STATEMENT_BLOCK_
#define _H_AST_STATEMENT_BLOCK_
#include "ast/node.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
ast_node_t read_ast_statement_block(allocator_t allocator, position_t *position,
                                    const char *end, const char *filename);
#ifdef __cplusplus
}
#endif
#endif