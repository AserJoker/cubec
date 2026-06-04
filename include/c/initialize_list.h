#ifndef _H_C_INITIALIZE_LIST_
#define _H_C_INITIALIZE_LIST_
#include "ast/node.h"
#include "c/writer.h"
#ifdef __cplusplus
extern "C" {
#endif
void c_initialize_list(c_writer_t writer, ast_node_t node);
#ifdef __cplusplus
}
#endif
#endif