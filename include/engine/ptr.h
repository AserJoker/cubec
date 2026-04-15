#ifndef _H_CUBEC_ENGINE_PTR_
#define _H_CUBEC_ENGINE_PTR_
#include "engine/context.h"
#include "engine/type.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
value_t create_ptr_type(context_t ctx, type_t type, bool mutable,
                        bool volatile_);
value_t create_ptr_array_type(context_t ctx, type_t type, bool mutable,
                              bool volatile_);
type_t ptr_type_get_type(type_t self);
bool ptr_type_is_mutable(type_t self);
bool ptr_type_is_volatile(type_t self);

#ifdef __cplusplus
}
#endif
#endif