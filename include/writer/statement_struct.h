#ifndef _H_WRITER_STATEMENT_STRUCT_
#define _H_WRITER_STATEMENT_STRUCT_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/stream.h"
#ifdef __cplusplus
extern "C" {
#endif
void write_statement_struct(allocator_t allocator, ast_node_t node,
                            stream_t stream);
#ifdef __cplusplus
}
#endif
#endif