#ifndef _H_AST_INITIALIZE_FIELD_
#define _H_AST_INITIALIZE_FIELD_
#include "ast/node.h"
#include "reader/token.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
ast_node_t read_initialize_field(allocator_t allocator,
                                     token_stream_t stream);
#ifdef __cplusplus
}
#endif
#endif