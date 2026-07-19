#ifndef _H_CUBEC_ENGINE_BUILTIN_DEBUG_
#define _H_CUBEC_ENGINE_BUILTIN_DEBUG_
#include "engine/builtin.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register debug-related builtins (assert).
 *        Must be called after checker's builtin types are initialized.
 */
void builtin_table_init_debug(builtin_table_t table, struct checker *ctx);

/**
 * @brief Comptime eval callback for assert(condition: bool): void.
 */
struct comptime_value *builtin_assert_eval(struct comptime_eval *eval,
                                           struct checker *ctx, node_t node,
                                           struct builtin_entry *be);

#ifdef __cplusplus
}
#endif
#endif
