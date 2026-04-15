#ifndef _H_CUBEC_ENGINE_BUILTIN_
#define _H_CUBEC_ENGINE_BUILTIN_
#include "engine/context.h"
#include "engine/value.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef value_t (*builtin_fn_t)(context_t ctx, size_t argc, value_t *argv);
void init_builtin_type(context_t ctx);
value_t create_builtin(context_t ctx, builtin_fn_t fn, const char *name);

#ifdef __cplusplus
}
#endif
#endif