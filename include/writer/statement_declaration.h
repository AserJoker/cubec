#ifndef _H_WRITER_STATEMENT_DECLARATION_
#define _H_WRITER_STATEMENT_DECLARATION_
#include "ast/node.h"
#include "core/string.h"
#ifdef __cplusplus
extern "C" {
#endif
void write_statement_declaration(allocator_t allocator, ast_node_t node,
                                 string_t out);
#ifdef __cplusplus
}
#endif
#endif