#ifndef _H_ENGINE_STR_
#define _H_ENGINE_STR_
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif
void init_str_type(context_t ctx);
value_t create_comptime_str(context_t ctx, const char *fmt, ...);
#ifdef __cplusplus
}
#endif
#endif