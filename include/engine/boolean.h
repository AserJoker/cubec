#ifndef _H_CUBEC_ENGINE_BOOLEAN_
#define _H_CUBEC_ENGINE_BOOLEAN_
#include "engine/context.h"
#include "engine/value.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void cubec_init_boolean_type(cubec_context_t ctx);
cubec_value_t cubec_create_boolean(cubec_context_t ctx, bool value,
                                   bool mutable, const char *name);
#ifdef __cplusplus
}
#endif
#endif