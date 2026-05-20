#ifndef _H_ENGINE_ARR_
#define _H_ENGINE_ARR_
#include "engine/context.h"
#include "engine/type.h"
#ifdef __cplusplus
extern "C" {
#endif
type_t create_arr_type(context_t ctx, type_t type, size_t length);
type_t arr_type_get_type(type_t type);
size_t arr_type_get_length(type_t type);
#ifdef __cplusplus
}
#endif
#endif