#ifndef _H_C_EXPRESSION_CALL_
#define _H_C_EXPRESSION_CALL_
#include "ast/node.h"
#include "c/writer.h"
#ifdef __cplusplus
extern "C" {
#endif
void c_expression_call(c_writer_t writer, ast_node_t node);
#ifdef __cplusplus
}
#endif
#endif