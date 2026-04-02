#ifndef _H_CUBEC_ENGINE_PTR_
#define _H_CUBEC_ENGINE_PTR_
#include "engine/context.h"
#include "engine/type.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
cubec_type_t cubec_create_array_type(cubec_context_t self, cubec_type_t type,
                                     size_t length);
cubec_type_t cubec_array_type_get_type(cubec_type_t self);
size_t cubec_array_type_get_length(cubec_type_t self);
#ifdef __cplusplus
}
#endif
#endif