#ifndef _H_AST_EXPRESSION_SLICE_
#define _H_AST_EXPRESSION_SLICE_
#include "ast/node.h"
#include "core/allocator.h"
#include "reader/token.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

ast_node_t read_expression_slice(allocator_t allocator,
                                     token_stream_t stream);

#ifdef __cplusplus
}
#endif
#endif