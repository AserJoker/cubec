#ifndef _H_CUBEC_ENGINE_CHECKER_CHECK_STMT_
#define _H_CUBEC_ENGINE_CHECKER_CHECK_STMT_
#include "engine/checker.h"
#include "core/node.h"
#include "engine/semantic_type.h"
#ifdef __cplusplus
extern "C" {
#endif

void _check_statement(checker_t ctx, node_t stmt, semantic_type_t return_type);
void checker_check_function_bodies(checker_t ctx, node_t program);

#ifdef __cplusplus
}
#endif
#endif
