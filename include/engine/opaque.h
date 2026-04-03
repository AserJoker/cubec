#ifndef _H_CUBEC_ENGINE_OPAQUE_
#define _H_CUBEC_ENGINE_OPAQUE_
#include "engine/context.h"
#include "engine/value.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void cubec_init_opaque_type(cubec_context_t ctx);
cubec_value_t cubec_create_opaque(cubec_context_t ctx, const void *data,
                                  bool mutable, const char *name);

#ifdef __cplusplus
}
#endif
#endif