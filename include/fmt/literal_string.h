#ifndef _H_FMT_LITERAL_STRING_
#define _H_FMT_LITERAL_STRING_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/stream.h"
#ifdef __cplusplus
extern "C" {
#endif
void fmt_literal_string(allocator_t allocator, ast_node_t ndoe,
                        stream_t stream);
#ifdef __cplusplus
}
#endif
#endif