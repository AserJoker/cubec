#ifndef _H_WRITER_INITIALIZE_FIELD_
#define _H_WRITER_INITIALIZE_FIELD_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/stream.h"
#ifdef __cplusplus
extern "C" {
#endif
void write_initialize_field(allocator_t allocator, ast_node_t node,
                            stream_t stream);
#ifdef __cplusplus
}
#endif
#endif