#ifndef _H_AST_EXPRESSION_GENERICS_
#define _H_AST_EXPRESSION_GENERICS_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/position.h"
#ifdef __cplusplus
extern "C" {
#endif

ast_node_t read_ast_expression_generics(allocator_t allocator,
                                        position_t *position, const char *end,
                                        const char *filename);
#ifdef __cplusplus
}
#endif
#endif