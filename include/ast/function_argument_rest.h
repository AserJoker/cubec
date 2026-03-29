#ifndef _H_CUBEC_AST_FUNCTION_ARGUMENT_REST_
#define _H_CUBEC_AST_FUNCTION_ARGUMENT_REST_
#include "ast/node.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
cubec_ast_node_t
cubec_read_ast_function_argument_rest(cubec_allocator_t allocator,
                                      cubec_position_t *position,
                                      const char *end, const char *filename);
#ifdef __cplusplus
}
#endif
#endif