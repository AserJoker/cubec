#ifndef _H_RESOLVE_STRUCT_FIELD_
#define _H_RESOLVE_STRUCT_FIELD_
#include "ast/node.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
value_t resolve_struct_field(context_t ctx, ast_node_t node);
#ifdef __cplusplus
}
#endif
#endif