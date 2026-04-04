#ifndef _H_CUBEC_ENGINE_ERROR_
#define _H_CUBEC_ENGINE_ERROR_
#include "engine/context.h"
#include "engine/value.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void cubec_init_error_type(cubec_context_t ctx);
cubec_value_t cubec_create_error(cubec_context_t ctx, const char *fmt, ...);

#ifdef __cplusplus
}
#endif
#endif