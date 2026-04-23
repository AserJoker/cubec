#ifndef _H_LITERAL_IDENTIFIER_
#define _H_LITERAL_IDENTIFIER_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/string.h"
#ifdef __cplusplus
extern "C" {
#endif
void write_literal_identifier(allocator_t allocator, ast_node_t node,
                              string_t out);
#ifdef __cplusplus
}
#endif
#endif