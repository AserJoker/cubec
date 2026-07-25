#ifndef _H_CUBEC_ENGINE_CHECKER_CHECK_STMT_
#define _H_CUBEC_ENGINE_CHECKER_CHECK_STMT_
#include "engine/context.h"
#include "engine/symbol.h"
#include "core/node.h"
#include "core/strmap.h"
#include "engine/semantic_type.h"
#include "engine/scope.h"
#ifdef __cplusplus
extern "C" {
#endif

flow_state_t _check_statement(context_t ctx, node_t stmt, semantic_type_t return_type);
void context_check_function_bodies(context_t ctx, node_t program);

/**
 * @brief Enqueue a function body for checking in the worklist.
 *        Deduplicates by cache key — if already checked, the entry is skipped.
 *        Takes ownership of type_bindings (frees on duplicate or when entry is processed).
 */
void _enqueue_body_check(context_t ctx, struct symbol *func_sym,
                          semantic_type_t inst_type, strmap_t type_bindings,
                          scope_t scope_root, bool is_method,
                          semantic_type_t host_type);

/**
 * @brief Run the full body checking pipeline (Pass 3 + Pass 4 combined).
 *        Enqueues non-generic functions/methods, then processes the worklist.
 *        Generic calls during body checking trigger new entries.
 */
void context_check_all_bodies(context_t ctx, node_t program);

#ifdef __cplusplus
}
#endif
#endif
