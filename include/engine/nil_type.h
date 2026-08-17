#ifndef _H_CUBEC_ENGINE_NIL_TYPE_
#define _H_CUBEC_ENGINE_NIL_TYPE_
#include "engine/type.h"
#ifdef __cplusplus
extern "C" {
#endif

/** @brief Get the "nil" type_t (static singleton, size=sizeof(void*)). */
type_t type_get_nil_type(allocator_t allocator);

/** @brief Create a nil value (data=NULL, initialized=true).
 *  Added to current_scope->values. */
struct _vm_t;
typedef struct _vm_t *vm_t;
value_t create_nil_value(vm_t vm);

#ifdef __cplusplus
}
#endif
#endif
