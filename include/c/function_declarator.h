#ifndef _H_C_FUNCTION_DECLARATOR_
#define _H_C_FUNCTION_DECLARATOR_
#include "ast/node.h"
#include "c/writer.h"
#ifdef __cplusplus
extern "C" {
#endif
void c_function_declarator(c_writer_t writer, ast_node_t node);
#ifdef __cplusplus
}
#endif
#endif