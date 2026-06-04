#ifndef _H_C_LITERAL_IDENTIFIER_
#define _H_C_LITERAL_IDENTIFIER_
#include "ast/node.h"
#include "c/writer.h"
#ifdef __cplusplus
extern "C" {
#endif
void c_literal_identifier(c_writer_t writer, ast_node_t node);
#ifdef __cplusplus
}
#endif
#endif