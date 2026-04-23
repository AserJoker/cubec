#ifndef _H_WRITER_FUNCTION_ARGUMENT_REST_
#define _H_WRITER_FUNCTION_ARGUMENT_REST_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/stream.h"
#ifdef __cplusplus
extern "C" {
#endif
void write_function_argument_rest(allocator_t allocator, ast_node_t node,
                                  stream_t stream);
#ifdef __cplusplus
}
#endif
#endif