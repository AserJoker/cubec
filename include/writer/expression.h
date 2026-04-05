#ifndef _H_CUBEC_WRITE_EXPRESSION_
#define _H_CUBEC_WRITE_EXPRESSION_
#include "ast/node.h"
#include "context.h"
#include <stdio.h>

#ifdef __cplusplus__
extern "C" {
#endif
void cubec_write_expression(FILE *fp, cubec_ast_node_t node,
                            cubec_write_context *ctx);
#ifdef __cplusplus__
}
#endif
#endif