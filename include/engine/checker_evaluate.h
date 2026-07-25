#ifndef _H_CUBEC_ENGINE_CHECKER_EVALUATE_
#define _H_CUBEC_ENGINE_CHECKER_EVALUATE_
#include "engine/context.h"
#include "core/node.h"
#ifdef __cplusplus
extern "C" {
#endif

void context_evaluate_declarations(context_t ctx, node_t program);
void context_evaluate_statement(context_t ctx, node_t stmt);
void context_evaluate_struct_union_members(context_t ctx, semantic_type_t t,
                                           vec_t members, size_t type_gp_count);

#ifdef __cplusplus
}
#endif
#endif
