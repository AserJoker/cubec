#ifndef _H_C_EXPRESSION_COMPUTE_MEMBER_
#define _H_C_EXPRESSION_COMPUTE_MEMBER_
#include "ast/node.h"
#include "c/writer.h"
#ifdef __cplusplus
extern "C" {
#endif
void c_expression_compute_member(c_writer_t writer, ast_node_t node);
#ifdef __cplusplus
}
#endif
#endif