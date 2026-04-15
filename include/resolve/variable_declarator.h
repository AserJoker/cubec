#ifndef _H_CUBEC_RESOLVE_VARIABLE_DECLARATOR_
#define _H_CUBEC_RESOLVE_VARIABLE_DECLARATOR_
#include "ast/node.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
value_t resolve_variable_declarator(context_t ctx, ast_node_t node);
#ifdef __cplusplus
}
#endif
#endif