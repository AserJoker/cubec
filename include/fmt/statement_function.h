#ifndef _H_FMT_STATEMENT_FUNCTION_
#define _H_FMT_STATEMENT_FUNCTION_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/stream.h"
#ifdef __cplusplus
extern "C" {
#endif
void fmt_statement_function(allocator_t allocator, ast_node_t node,
                            stream_t stream);
#ifdef __cplusplus
}
#endif
#endif