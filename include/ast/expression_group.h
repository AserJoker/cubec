#ifndef _H_AST_EXPRESSION_GROUP_
#define _H_AST_EXPRESSION_GROUP_
#include "ast/node.h"
#include "core/allocator.h"
#include "reader/token.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

ast_node_t read_expression_group(allocator_t allocator,
                                     token_stream_t stream);

ast_node_t ast_unwrap_group(ast_node_t node);

#ifdef __cplusplus
}
#endif
#endif