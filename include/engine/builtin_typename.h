#ifndef _H_CUBEC_ENGINE_BUILTIN_TYPENAME_
#define _H_CUBEC_ENGINE_BUILTIN_TYPENAME_
#include "engine/builtin.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register typename builtin (builtin func typename[T](): str).
 *        Returns the readable name of type T at compile time.
 */
void builtin_table_init_typename(builtin_table_t table, struct checker *ctx);

/**
 * @brief Comptime eval callback for typename[T](): str.
 */
struct comptime_value *builtin_typename_eval(struct comptime_eval *eval,
                                              struct checker *ctx, node_t node,
                                              struct builtin_entry *be);

#ifdef __cplusplus
}
#endif
#endif
