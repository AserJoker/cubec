#ifndef _H_CUBEC_ENGINE_BOOLEAN_
#define _H_CUBEC_ENGINE_BOOLEAN_
#include "engine/context.h"
#include "engine/value.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void init_boolean_type(context_t ctx);
value_t create_boolean(context_t ctx, bool value, bool mutable,
                       const char *name);
#ifdef __cplusplus
}
#endif
#endif