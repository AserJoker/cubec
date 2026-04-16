#ifndef _H_CUBEC_EVAL_STATEMENT_EXPRESSION_
#define _H_CUBEC_EVAL_STATEMENT_EXPRESSION_
#include "ast/node.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
value_t eval_statement_expression(context_t ctx, ast_node_t node);
#ifdef __cplusplus
}
#endif
#endif