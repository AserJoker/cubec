#ifndef _H_ENGINE_VOID_
#define _H_ENGINE_VOID_
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif
void init_void_type(context_t ctx);
value_t create_comptime_void(context_t ctx);
#ifdef __cplusplus
}
#endif
#endif