#ifndef _H_CUBEC_AST_LITERAL_SYMBOL_
#define _H_CUBEC_AST_LITERAL_SYMBOL_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/position.h"
#ifdef __cplusplus
extern "C" {
#endif

ast_node_t read_ast_literal_symbol(allocator_t allocator, position_t *position,
                                   const char *end, const char *filename);

#ifdef __cplusplus
}
#endif
#endif