#ifndef _H_ENGINE_ARRAY_
#define _H_ENGINE_ARRAY_
#include "engine/context.h"
#include "engine/type.h"
#ifdef __cplusplus
extern "C" {
#endif
type_t create_array_type(context_t ctx, type_t type, size_t length);
size_t array_type_get_length(type_t self);
type_t array_type_get_type(type_t self);
#ifdef __cplusplus
}
#endif
#endif