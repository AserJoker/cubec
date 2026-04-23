#ifndef _H_WRITER_STATEMENT_RETURN_
#define _H_WRITER_STATEMENT_RETURN_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/stream.h"
#ifdef __cplusplus
extern "C" {
#endif
void write_statement_return(allocator_t allocator, ast_node_t node,
                            stream_t stream);
#ifdef __cplusplus
}
#endif
#endif