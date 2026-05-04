#ifndef _H_FMT_PROGRAM_
#define _H_FMT_PROGRAM_
#include "ast/node.h"
#include "core/stream.h"
#ifdef __cplusplus
extern "C" {
#endif
void fmt_program(allocator_t allocator, ast_node_t node, stream_t stream);
#ifdef __cplusplus
}
#endif
#endif