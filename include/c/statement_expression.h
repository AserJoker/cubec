#ifndef _H_C_STATEMENT_EXPRESSION_
#define _H_C_STATEMENT_EXPRESSION_
#include "ast/node.h"
#include "c/writer.h"
#ifdef __cplusplus
extern "C" {
#endif
void c_statement_expression(c_writer_t writer, ast_node_t node);
#ifdef __cplusplus
}
#endif
#endif