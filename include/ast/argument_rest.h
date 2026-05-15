#ifndef _H_AST_FUNCTION_ARGUMENT_REST_
#define _H_AST_FUNCTION_ARGUMENT_REST_
#include "ast/node.h"
#include "reader/token.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
ast_node_t read_argument_rest(allocator_t allocator,
                                           token_stream_t stream);
#ifdef __cplusplus
}
#endif
#endif