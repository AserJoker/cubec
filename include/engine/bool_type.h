#ifndef _H_CUBEC_ENGINE_BOOL_TYPE_
#define _H_CUBEC_ENGINE_BOOL_TYPE_
#include "engine/type.h"
#ifdef __cplusplus
extern "C" {
#endif

/** @brief Create the "bool" type_t (static singleton). */
type_t type_create_bool_type(allocator_t allocator);

/** @brief Create a bool value. data=bool (1 byte). Added to current_scope->values. */
value_t create_bool_value(vm_t vm, bool val);

#ifdef __cplusplus
}
#endif
#endif
