#ifndef _H_CUBEC_ENGINE_BUILTIN_
#define _H_CUBEC_ENGINE_BUILTIN_
#include "engine/context.h"
#include "engine/value.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef cubec_value_t (*cubec_builtin_fn_t)(cubec_context_t ctx, size_t argc,
                                            cubec_value_t *argv);
void cubec_init_builtin_type(cubec_context_t ctx);
cubec_value_t cubec_create_builtin(cubec_context_t ctx, cubec_builtin_fn_t fn,
                                   const char *name);

#ifdef __cplusplus
}
#endif
#endif