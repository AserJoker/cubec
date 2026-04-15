#ifndef _H_CUBEC_ENGINE_OPAQUE_
#define _H_CUBEC_ENGINE_OPAQUE_
#include "engine/context.h"
#include "engine/value.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void init_opaque_type(context_t ctx);
value_t create_opaque(context_t ctx, const void *data, bool mutable,
                      const char *name);

#ifdef __cplusplus
}
#endif
#endif