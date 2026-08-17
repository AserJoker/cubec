#ifndef _H_CUBEC_ENGINE_OPAQUE_TYPE_
#define _H_CUBEC_ENGINE_OPAQUE_TYPE_
#include "engine/type.h"
#ifdef __cplusplus
extern "C" {
#endif

/** @brief Get the "opaque" type_t (static singleton, size=sizeof(void*)).
 *  Opaque is the C equivalent of void* — an untyped pointer. */
type_t type_get_opaque_type(allocator_t allocator);

/** @brief Create an opaque value wrapping an arbitrary address.
 *  data = void* (owned copy of the address). Added to current_scope->values. */
struct _vm_t;
typedef struct _vm_t *vm_t;
value_t create_opaque_value(vm_t vm, void *addr);

#ifdef __cplusplus
}
#endif
#endif
