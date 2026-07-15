#ifndef _H_CUBEC_ENGINE_CHECKER_CHECK_EXPR_HELPERS_
#define _H_CUBEC_ENGINE_CHECKER_CHECK_EXPR_HELPERS_
#include "engine/checker.h"
#include "core/node.h"
#include "engine/semantic_type.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Helpers for checker_check_expr.c — not part of public API */
semantic_type_t _check_assign_generic_lhs(checker_t ctx, node_t expr,
                                           semantic_type_t lt,
                                           semantic_type_t rt);
semantic_type_t _check_generic_ident_callee(checker_t ctx, node_t expr);
void _check_init_list_named_fields(checker_t ctx, node_t expr,
                                    semantic_type_t t, vec_t fields,
                                    size_t fcount, size_t icount, vec_t items);
void _check_init_list_positional(checker_t ctx, node_t expr,
                                  semantic_type_t t, vec_t fields,
                                  size_t fcount, size_t icount, vec_t items);
semantic_type_t _check_binary_arithmetic(checker_t ctx, node_t expr,
                                          const char *op,
                                          semantic_type_t lt,
                                          semantic_type_t rt);
semantic_type_t _check_binary_bitwise(checker_t ctx, node_t expr,
                                       const char *op,
                                       semantic_type_t lt,
                                       semantic_type_t rt);
bool _is_op_one_of(const char *op, const char **ops, size_t count);
void _check_func_params(checker_t ctx, void *fn_node, vec_t param_types);

#ifdef __cplusplus
}
#endif
#endif
