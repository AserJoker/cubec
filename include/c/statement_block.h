#ifndef _H_C_STATEMENT_BLOCK_
#define _H_C_STATEMENT_BLOCK_
#include "ast/node.h"
#include "c/writer.h"
#ifdef __cplusplus
extern "C" {
#endif
void c_statement_block(c_writer_t writer, ast_node_t node);
#ifdef __cplusplus
}
#endif
#endif