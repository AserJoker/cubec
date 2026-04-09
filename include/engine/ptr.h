#ifndef _H_CUBEC_ENGINE_PTR_
#define _H_CUBEC_ENGINE_PTR_
#include "engine/context.h"
#include "engine/type.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
cubec_value_t cubec_create_ptr_type(cubec_context_t ctx, cubec_type_t type,
                                    bool mutable, bool volatile_);
cubec_value_t cubec_create_ptr_array_type(cubec_context_t ctx,
                                          cubec_type_t type, bool mutable,
                                          bool volatile_);
cubec_type_t cubec_ptr_type_get_type(cubec_type_t self);
bool cubec_ptr_type_is_mutable(cubec_type_t self);
bool cubec_ptr_type_is_volatile(cubec_type_t self);

#ifdef __cplusplus
}
#endif
#endif