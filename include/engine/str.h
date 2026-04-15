#ifndef _H_CUBEC_ENGINE_STR_
#define _H_CUBEC_ENGINE_STR_
#include "engine/context.h"
#include "engine/value.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void init_str_type(context_t ctx);
value_t create_str(context_t ctx, const char *data, const char *name);

#ifdef __cplusplus
}
#endif
#endif