#ifndef _H_CUBEC_ENGINE_BUILTIN_DISPATCH_
#define _H_CUBEC_ENGINE_BUILTIN_DISPATCH_
#include "engine/builtin.h"
#ifdef __cplusplus
extern "C" {
#endif

/* Builtin function eval callbacks — implemented in src/engine/builtin_dispatch.c */

struct comptime_value *builtin_assert_eval(struct comptime_eval *eval,
                                         struct checker *ctx, node_t node,
                                         struct builtin_entry *be);

struct comptime_value *builtin_length_eval(struct comptime_eval *eval,
                                         struct checker *ctx, node_t node,
                                         struct builtin_entry *be);

struct comptime_value *builtin_get_eval(struct comptime_eval *eval,
                                      struct checker *ctx, node_t node,
                                      struct builtin_entry *be);

struct comptime_value *builtin_set_eval(struct comptime_eval *eval,
                                      struct checker *ctx, node_t node,
                                      struct builtin_entry *be);

#ifdef __cplusplus
}
#endif
#endif
