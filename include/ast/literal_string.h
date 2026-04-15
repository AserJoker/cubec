#ifndef _H_CUBEC_AST_LITERAL_STRING_
#define _H_CUBEC_AST_LITERAL_STRING_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/position.h"
#ifdef __cplusplus
extern "C" {
#endif

ast_node_t read_ast_literal_string(allocator_t allocator, position_t *position,
                                   const char *end, const char *filename);
#ifdef __cplusplus
}
#endif
#endif