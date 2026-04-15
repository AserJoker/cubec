#ifndef _H_CUBEC_ENGINE_ARRAY_
#define _H_CUBEC_ENGINE_ARRAY_
#include "engine/context.h"
#include "engine/type.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
value_t create_array_type(context_t self, type_t type, size_t length);
type_t array_type_get_type(type_t self);
size_t array_type_get_length(type_t self);
#ifdef __cplusplus
}
#endif
#endif