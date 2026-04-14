#ifndef _H_CUBEC_RESOLVE_TYPE_
#define _H_CUBEC_RESOLVE_TYPE_
#include "ast/node.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t cubec_resolve_type(cubec_context_t ctx, cubec_ast_node_t node);
#ifdef __cplusplus
}
#endif
#endif