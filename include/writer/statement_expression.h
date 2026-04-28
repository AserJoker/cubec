#ifndef _H_WRITER_STATEMENT_EXPRESSION_
#define _H_WRITER_STATEMENT_EXPRESSION_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/stream.h"
#ifdef __cplusplus
extern "C" {
#endif
void write_statement_expression(allocator_t allocator, ast_node_t node,
                                stream_t stream);
#ifdef __cplusplus
}
#endif
#endif