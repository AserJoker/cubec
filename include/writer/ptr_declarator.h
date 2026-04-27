#ifndef _H_WRITER_PTR_DECLARATOR_
#define _H_WRITER_PTR_DECLARATOR_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/stream.h"
#ifdef __cplusplus
extern "C" {
#endif
void write_ptr_declarator(allocator_t allocator, ast_node_t node,
                          stream_t stream);
#ifdef __cplusplus
}
#endif
#endif