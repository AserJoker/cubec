#ifndef _H_SLICE_DECLARATOR_
#define _H_SLICE_DECLARATOR_
#include "ast/node.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
value_t resolve_slice_declarator(context_t ctx, ast_node_t node);
#ifdef __cplusplus
}
#endif
#endif