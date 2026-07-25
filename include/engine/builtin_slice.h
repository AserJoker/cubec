#ifndef _H_CUBEC_ENGINE_BUILTIN_SLICE_
#define _H_CUBEC_ENGINE_BUILTIN_SLICE_
#include "engine/builtin.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register slice-related builtins (makeSlice).
 *        Must be called after checker's builtin types are initialized.
 */
void builtin_table_init_slice(builtin_table_t table, struct context *ctx);

/**
 * @brief Comptime eval callback for makeSlice[T](pointer:*T, start:u64, len:u64):[]T.
 */
struct comptime_value *builtin_makeSlice_eval(struct comptime_eval *eval,
                                               struct context *ctx, node_t node,
                                               struct builtin_entry *be);

#ifdef __cplusplus
}
#endif
#endif
