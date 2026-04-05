#ifndef _H_CUBEC_WRITE_VARIABLE_DECLARATOR_
#define _H_CUBEC_WRITE_VARIABLE_DECLARATOR_
#include "ast/node.h"
#include "context.h"
#include <stdio.h>

#ifdef __cplusplus__
extern "C" {
#endif
void cubec_write_variable_declarator(FILE *fp, cubec_ast_node_t node,
                                     cubec_write_context *ctx);
#ifdef __cplusplus__
}
#endif
#endif