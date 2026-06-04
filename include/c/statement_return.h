#ifndef _H_C_STATEMENT_RETURN_
#define _H_C_STATEMENT_RETURN_
#include "ast/node.h"
#include "c/writer.h"
#ifdef __cplusplus
extern "C" {
#endif
void c_statement_return(c_writer_t writer, ast_node_t node);
#ifdef __cplusplus
}
#endif
#endif