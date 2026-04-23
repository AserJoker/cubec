#ifndef _H_WRITER_PROGRAM_
#define _H_WRITER_PROGRAM_
#include "ast/node.h"
#include "core/stream.h"
#ifdef __cplusplus
extern "C" {
#endif
void write_program(allocator_t allocator, ast_node_t node, stream_t stream);
#ifdef __cplusplus
}
#endif
#endif