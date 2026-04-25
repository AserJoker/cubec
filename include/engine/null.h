#ifndef _H_ENGINE_NULL_
#define _H_ENGINE_NULL_
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
void null_init(context_t ctx);
value_t create_null(context_t ctx, bool mut, const char *name);
value_t create_comptime_null(context_t ctx, bool mut, const char *name);
#ifdef __cplusplus
}
#endif
#endif