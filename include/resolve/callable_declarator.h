#ifndef _H_RESOLVE_CALLABLE_DECLARATOR_
#define _H_RESOLVE_CALLABLE_DECLARATOR_
#include "ast/node.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif
value_t resolve_callable_declarator(context_t ctx, ast_node_t node);
#ifdef __cplusplus
}
#endif
#endif