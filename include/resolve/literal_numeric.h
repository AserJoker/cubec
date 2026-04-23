#ifndef _H_RESOLVE_LITERAL_NUMERIC_
#define _H_RESOLVE_LITERAL_NUMERIC_
#include "ast/node.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
value_t resolve_literal_numeric(context_t ctx, ast_node_t node);
#ifdef __cplusplus
}
#endif
#endif