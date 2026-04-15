#ifndef _H_CUBEC_AST_INITIALIZE_LIST_
#define _H_CUBEC_AST_INITIALIZE_LIST_
#include "ast/node.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
ast_node_t read_ast_initialize_list(allocator_t allocator, position_t *position,
                                    const char *end, const char *filename);
#ifdef __cplusplus
}
#endif
#endif