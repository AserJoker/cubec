#ifndef _H_WRITER_EXPRESSION_SPREAD_
#define _H_WRITER_EXPRESSION_SPREAD_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/stream.h"
#ifdef __cplusplus
extern "C" {
#endif
void write_expression_spread(allocator_t allocator, ast_node_t node,
                             stream_t stream);
#ifdef __cplusplus
}
#endif
#endif