#ifndef _H_C_STATEMENT_STRUCT_
#define _H_C_STATEMENT_STRUCT_
#include "ast/node.h"
#include "c/writer.h"
#ifdef __cplusplus
extern "C" {
#endif
void c_statement_struct(c_writer_t writer, ast_node_t node);
#ifdef __cplusplus
}
#endif
#endif