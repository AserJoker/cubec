#ifndef _H_CUBEC_WRITE_INITIALIZE_FIELD_
#define _H_CUBEC_WRITE_INITIALIZE_FIELD_
#include "ast/node.h"
#include "context.h"
#include <stdio.h>

#ifdef __cplusplus__
extern "C" {
#endif
void cubec_write_initialize_field(FILE *fp, cubec_ast_node_t node,
                                  cubec_write_context *ctx);
#ifdef __cplusplus__
}
#endif
#endif