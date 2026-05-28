#ifndef _H_RESOLVE_LITERAL_CHAR_
#define _H_RESOLVE_LITERAL_CHAR_
#include "ast/node.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
value_t resolve_literal_char(context_t ctx, ast_node_t node);
#ifdef __cplusplus
}
#endif
#endif