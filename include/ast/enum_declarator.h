#ifndef _H_CUBEC_AST_ENUM_DECLARATOR_
#define _H_CUBEC_AST_ENUM_DECLARATOR_
#include "ast/node.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
ast_node_t read_ast_enum_declarator(allocator_t allocator, position_t *position,
                                    const char *end, const char *filename);
#ifdef __cplusplus
}
#endif
#endif