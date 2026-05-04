#ifndef _H_FMT_INITIALIZE_FIELD_
#define _H_FMT_INITIALIZE_FIELD_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/stream.h"
#ifdef __cplusplus
extern "C" {
#endif
void fmt_initialize_field(allocator_t allocator, ast_node_t node,
                          stream_t stream);
#ifdef __cplusplus
}
#endif
#endif