#ifndef _H_WRITER_VALUE_
#define _H_WRITER_VALUE_
#include "ast/node.h"
#include "core/stream.h"
#ifdef __cplusplus
extern "C" {
#endif
void write_value(allocator_t allocator, ast_node_t node, stream_t stream);
#ifdef __cplusplus
}
#endif
#endif