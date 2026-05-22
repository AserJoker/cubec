#ifndef _H_RESOLVE_STATEMENT_STRUCT_
#define _H_RESOLVE_STATEMENT_STRUCT_
#include "ast/node.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
value_t resolve_statement_struct(context_t ctx, ast_node_t node);
#ifdef __cplusplus
}
#endif
#endif