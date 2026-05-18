#ifndef _H_ENGINE_ERROR_
#define _H_ENGINE_ERROR_
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _error_t *error_t;
void init_error_type(context_t ctx);
value_t create_error(context_t ctx, const char *fmt, ...);
#ifdef __cplusplus
}
#endif
#endif