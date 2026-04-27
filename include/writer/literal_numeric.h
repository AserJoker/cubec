#ifndef _H_WRITER_LITERAL_NUMBERIC_
#define _H_WRITER_LITERAL_NUMBERIC_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/stream.h"
#ifdef __cplusplus
extern "C" {
#endif
void write_literal_numeric(allocator_t allocator, ast_node_t node,
                           stream_t stream);
#ifdef __cplusplus
}
#endif
#endif