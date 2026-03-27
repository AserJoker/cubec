#ifndef _H_CUBEC_EVAL_STATEMENT_BLOCK_
#define _H_CUBEC_EVAL_STATEMENT_BLOCK_
#include "ast/statement_block.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cpluplus
extern "C" {
#endif
cubec_value_t cubec_eval_statement_block(cubec_context_t ctx,
                                         cubec_ast_node_t sts,
                                         const char *filename);
#ifdef __cpluplus
}
#endif
#endif