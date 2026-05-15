#ifndef _H_AST_EXPRESSION_MEMBER_
#define _H_AST_EXPRESSION_MEMBER_
#include "ast/node.h"
#include "core/allocator.h"
#include "reader/token.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

ast_node_t read_expression_member(allocator_t allocator,
                                      token_stream_t stream);

#ifdef __cplusplus
}
#endif
#endif