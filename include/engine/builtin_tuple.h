#ifndef _H_CUBEC_ENGINE_BUILTIN_TUPLE_
#define _H_CUBEC_ENGINE_BUILTIN_TUPLE_
#include "engine/builtin.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register tuple-related builtins (getTupleItem, setTupleItem).
 *        Must be called after checker's builtin types are initialized.
 */
void builtin_table_init_tuple(builtin_table_t table, struct checker *ctx);

/**
 * @brief Comptime eval callback for getTupleItem[N: u64, ...Args](tuple: <...Args>): Args[N].
 */
struct comptime_value *builtin_get_eval(struct comptime_eval *eval,
                                        struct checker *ctx, node_t node,
                                        struct builtin_entry *be);

/**
 * @brief Comptime eval callback for setTupleItem[N: u64, ...Args](tuple: <...Args>, value: Args[N]): void.
 */
struct comptime_value *builtin_set_eval(struct comptime_eval *eval,
                                        struct checker *ctx, node_t node,
                                        struct builtin_entry *be);

#ifdef __cplusplus
}
#endif
#endif
