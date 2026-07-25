#ifndef _H_CUBEC_ENGINE_BUILTIN_STRING_
#define _H_CUBEC_ENGINE_BUILTIN_STRING_
#include "engine/builtin.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register string-related builtins (toString).
 *        Must be called after checker's builtin types are initialized.
 */
void builtin_table_init_string(builtin_table_t table, struct context *ctx);

/**
 * @brief Comptime eval callback for toString[T](obj: T): str.
 */
struct comptime_value *builtin_toString_eval(struct comptime_eval *eval,
                                            struct context *ctx, node_t node,
                                            struct builtin_entry *be);

#ifdef __cplusplus
}
#endif
#endif
