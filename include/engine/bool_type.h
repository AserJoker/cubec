#ifndef _H_CUBEC_ENGINE_BOOL_TYPE_
#define _H_CUBEC_ENGINE_BOOL_TYPE_
#include "engine/type.h"
#ifdef __cplusplus
extern "C" {
#endif

/** @brief Get the "bool" type_t (static singleton, mut=true). */
type_t type_get_bool_type(allocator_t allocator);

/** @brief Get the "const bool" type_t (static singleton, mut=false). */
type_t type_get_const_bool_type(allocator_t allocator);

/** @brief Create a bool value. data=bool (1 byte). Added to current_scope->values. */
value_t create_bool_value(vm_t vm, bool val);

#ifdef __cplusplus
}
#endif
#endif
