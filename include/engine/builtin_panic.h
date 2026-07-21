#ifndef _H_CUBEC_ENGINE_BUILTIN_PANIC_
#define _H_CUBEC_ENGINE_BUILTIN_PANIC_
#include "engine/builtin.h"
#ifdef __cplusplus
extern "C" {
#endif

void builtin_table_init_panic(builtin_table_t table, struct checker *ctx);
struct comptime_value *builtin_panic_eval(struct comptime_eval *eval,
                                          struct checker *ctx, node_t node,
                                          struct builtin_entry *be);

#ifdef __cplusplus
}
#endif
#endif
