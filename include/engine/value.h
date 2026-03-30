#ifndef _H_CUBEC_ENGINE_VALUE_
#define _H_CUBEC_ENGINE_VALUE_
#include "core/allocator.h"
#include "engine/type.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_value_t *cubec_value_t;
cubec_value_t cubec_create_value(cubec_allocator_t allocator, cubec_type_t type,
                                 bool mutable, const void *data);
cubec_type_t cubec_value_get_type(cubec_value_t value);
bool cubec_value_is_mutable(cubec_value_t value);
void *cubec_value_get_data(cubec_value_t value);
cubec_value_t cubec_value_clone(cubec_allocator_t allocator,
                                cubec_value_t value);
bool cubec_value_is_error(cubec_value_t value);
#ifdef __cplusplus
}
#endif
#endif