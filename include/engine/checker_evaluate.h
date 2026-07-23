#ifndef _H_CUBEC_ENGINE_CHECKER_EVALUATE_
#define _H_CUBEC_ENGINE_CHECKER_EVALUATE_
#include "engine/checker.h"
#include "core/node.h"
#ifdef __cplusplus
extern "C" {
#endif

void checker_evaluate_declarations(checker_t ctx, node_t program);
void checker_evaluate_statement(checker_t ctx, node_t stmt);
void checker_evaluate_struct_union_members(checker_t ctx, semantic_type_t t,
                                           vec_t members, size_t type_gp_count);

#ifdef __cplusplus
}
#endif
#endif
