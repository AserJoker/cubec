#ifndef _H_ENGINE_STR_
#define _H_ENGINE_STR_
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
void str_init(context_t ctx);
value_t create_str(context_t ctx, const char *src);
#ifdef __cplusplus
}
#endif
#endif