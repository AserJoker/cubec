#ifndef _H_LITERAL_IDENTIFIER_
#define _H_LITERAL_IDENTIFIER_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/stream.h"
#ifdef __cplusplus
extern "C" {
#endif
void fmt_literal_identifier(allocator_t allocator, ast_node_t node,
                            stream_t stream);
#ifdef __cplusplus
}
#endif
#endif