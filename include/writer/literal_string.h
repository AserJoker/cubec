#ifndef _H_CUBEC_WRITE_LITERAL_STRING_
#define _H_CUBEC_WRITE_LITERAL_STRING_
#include "ast/node.h"
#include "context.h"
#include <stdio.h>

#ifdef __cplusplus__
extern "C" {
#endif
void cubec_write_literal_string(FILE *fp, cubec_ast_node_t node,
                                cubec_write_context *ctx);
#ifdef __cplusplus__
}
#endif
#endif