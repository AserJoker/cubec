#ifndef _H_STATEMENT_FUNCTION_
#define _H_STATEMENT_FUNCTION_
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

value_t resolve_statement_function(context_t ctx, ast_node_t node);
#ifdef __cplusplus
}
#endif
#endif