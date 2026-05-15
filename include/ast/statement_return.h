#ifndef _H_AST_STATEMENT_RETURN_
#define _H_AST_STATEMENT_RETURN_
#include "ast/node.h"
#include "reader/token.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
ast_node_t read_statement_return(allocator_t allocator,
                                     token_stream_t stream);
#ifdef __cplusplus
}
#endif
#endif