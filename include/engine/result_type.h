#ifndef _H_CUBEC_ENGINE_RESULT_TYPE_
#define _H_CUBEC_ENGINE_RESULT_TYPE_
#include "engine/type.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _vm_t;
typedef struct _vm_t *vm_t;

/**
 * @brief Create a result[T,E] union type with _value:T, _error:E and
 *  builtin methods: ok, value, error (instance) and of_value, of_error (static).
 *  The union_type_t is registered in current_scope->types.
 *  Returns the type value (value.data = union_type_t, own=false).
 *  module_id defaults to current_module_id. */
value_t vm_create_result_type_value(vm_t self, type_t T, type_t E);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_RESULT_TYPE_ */
