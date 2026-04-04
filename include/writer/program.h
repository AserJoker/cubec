#ifndef _H_CUBEC_WRITE_PROGRAM_
#define _H_CUBEC_WRITE_PROGRAM_
#include "ast/node.h"
#include "context.h"
#include <stdio.h>

#ifdef __cplusplus__
extern "C" {
#endif
void cubec_write_program(FILE *fp, cubec_ast_node_t node,
                         cubec_write_context *ctx);
#ifdef __cplusplus__
}
#endif
#endif