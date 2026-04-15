#ifndef _H_CUBEC_RESOLVE_EXPRESSION_
#define _H_CUBEC_RESOLVE_EXPRESSION_
#include "ast/node.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
value_t resolve_expression(context_t ctx, ast_node_t node);
#ifdef __cplusplus
}
#endif
#endif