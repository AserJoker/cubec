#ifndef _H_WRITE_FUNCTION_BODY_
#define _H_WRITE_FUNCTION_BODY_
#include "ast/node.h"
#include "core/stream.h"
#ifdef __cplusplus
extern "C" {
#endif
void fmt_function_body(allocator_t allocator, ast_node_t node, stream_t stream);
#ifdef __cplusplus
}
#endif
#endif