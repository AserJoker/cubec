#ifndef _H_CUBEC_ENGINE_COMPTIME_EVAL_BINARY_
#define _H_CUBEC_ENGINE_COMPTIME_EVAL_BINARY_
#include "engine/comptime_eval_internal.h"
#ifdef __cplusplus
extern "C" {
#endif

comptime_value_t _comptime_eval_binary(comptime_eval_t eval, context_t ctx,
                                        node_t node);

#ifdef __cplusplus
}
#endif
#endif
