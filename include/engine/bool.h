#ifndef _H_ENGINE_BOOL_
#define _H_ENGINE_BOOL_
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
void init_bool_type(context_t ctx);
value_t create_comptime_bool(context_t ctx, bool value, bool mut,
                             const char *name);
value_t create_bool(context_t ctx, bool mut, const char *name);
#ifdef __cplusplus
}
#endif
#endif