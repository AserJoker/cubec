#ifndef _H_CUBEC_ENGINE_BUILTIN_COLLECTION_
#define _H_CUBEC_ENGINE_BUILTIN_COLLECTION_
#include "engine/builtin.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register collection-related builtins (length).
 *        Must be called after checker's builtin types are initialized.
 */
void builtin_table_init_collection(builtin_table_t table, struct context *ctx);

/**
 * @brief Comptime eval callback for length[T](list: T): u64.
 */
struct comptime_value *builtin_length_eval(struct comptime_eval *eval,
                                           struct context *ctx, node_t node,
                                           struct builtin_entry *be);

#ifdef __cplusplus
}
#endif
#endif
