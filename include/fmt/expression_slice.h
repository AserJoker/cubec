#ifndef _H_FMT_EXPRESSION_SLICE_
#define _H_FMT_EXPRESSION_SLICE_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/stream.h"
#ifdef __cplusplus
extern "C" {
#endif
void fmt_expression_slice(allocator_t allocator, ast_node_t node,
                          stream_t stream);
#ifdef __cplusplus
}
#endif
#endif