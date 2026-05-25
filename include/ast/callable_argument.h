#ifndef _H_AST_CALLABLE_ARGUMENT_
#define _H_AST_CALLABLE_ARGUMENT_
#include "ast/node.h"
#include "reader/token.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
ast_node_t read_callable_argument(allocator_t allocator, token_stream_t stream);
#ifdef __cplusplus
}
#endif
#endif