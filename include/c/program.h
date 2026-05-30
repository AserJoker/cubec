#ifndef _H_C_PROGRAM_
#define _H_C_PROGRAM_
#include "ast/node.h"
#include "c/writer.h"
#ifdef __cplusplus
extern "C" {
#endif
void c_program(c_writer_t writer, ast_node_t node);
#ifdef __cplusplus
}
#endif
#endif