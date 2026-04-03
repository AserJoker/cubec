#ifndef _H_CUBEC_ENGINE_STR_
#define _H_CUBEC_ENGINE_STR_
#include "engine/context.h"
#include "engine/value.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void cubec_init_str_type(cubec_context_t ctx);
cubec_value_t cubec_create_str(cubec_context_t ctx, const char *data,
                               const char *name);

#ifdef __cplusplus
}
#endif
#endif