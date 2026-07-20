#ifndef _H_CUBEC_ENGINE_BUILTIN_UNION_
#define _H_CUBEC_ENGINE_BUILTIN_UNION_
#include "engine/builtin.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register unionIs builtin (func unionIs[T,K](obj:K):bool).
 *        Must be called after checker's builtin types are initialized.
 */
void builtin_table_init_union(builtin_table_t table, struct checker *ctx);

/**
 * @brief Comptime eval callback for unionIs[T,K](obj:K):bool.
 */
struct comptime_value *builtin_unionis_eval(struct comptime_eval *eval,
                                            struct checker *ctx, node_t node,
                                            struct builtin_entry *be);

#ifdef __cplusplus
}
#endif
#endif
