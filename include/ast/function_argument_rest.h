#ifndef _H_AST_FUNCTION_ARGUMENT_REST_
#define _H_AST_FUNCTION_ARGUMENT_REST_
#include "ast/node.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
ast_node_t read_ast_function_argument_rest(allocator_t allocator,
                                           position_t *position,
                                           const char *end,
                                           const char *filename);
#ifdef __cplusplus
}
#endif
#endif