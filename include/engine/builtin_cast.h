#ifndef _H_CUBEC_ENGINE_BUILTIN_CAST_
#define _H_CUBEC_ENGINE_BUILTIN_CAST_
#include "engine/builtin.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register cast builtin (func cast[T,K](expr:K):T).
 *        Must be called after checker's builtin types are initialized.
 */
void builtin_table_init_cast(builtin_table_t table, struct context *ctx);

/**
 * @brief Comptime eval callback for cast[T,K](expr:K):T.
 */
struct comptime_value *builtin_cast_eval(struct comptime_eval *eval,
                                         struct context *ctx, node_t node,
                                         struct builtin_entry *be);

#ifdef __cplusplus
}
#endif
#endif
