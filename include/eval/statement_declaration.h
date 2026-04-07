#ifndef _H_CUBEC_EVAL_STATEMENT_DECLARATION_
#define _H_CUBEC_EVAL_STATEMENT_DECLARATION_
#include "ast/node.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus__
extern "C" {
#endif
cubec_value_t cubec_eval_statement_declaration(cubec_context_t ctx,
                                               cubec_ast_node_t node);
#ifdef __cplusplus__
}
#endif
#endif