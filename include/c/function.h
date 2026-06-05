#ifndef _H_C_FUNCTION_
#define _H_C_FUNCTION_
#include "ast/node.h"
#include "c/writer.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
void c_function_declar(c_writer_t writer, value_t func);
void c_function_declaration(c_writer_t writer, value_t func);
void c_function_closure(c_writer_t writer, value_t function);
void c_closure_declar(c_writer_t writer, ast_node_t node);
#ifdef __cplusplus
}
#endif
#endif