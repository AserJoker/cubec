#ifndef _H_CUBEC_ENGINE_CHECKER_CHECK_EXPR_
#define _H_CUBEC_ENGINE_CHECKER_CHECK_EXPR_
#include "engine/checker.h"
#include "core/node.h"
#include "engine/semantic_type.h"
#ifdef __cplusplus
extern "C" {
#endif

semantic_type_t _check_expression(checker_t ctx, node_t expr);
semantic_type_t checker_check_expression(checker_t ctx, node_t expr);

#ifdef __cplusplus
}
#endif
#endif
