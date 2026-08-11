#ifndef _H_CUBEC_ENGINE_VOID_TYPE_
#define _H_CUBEC_ENGINE_VOID_TYPE_
#include "engine/type.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _vm_t;
typedef struct _vm_t *vm_t;

/** @brief Get the "void" type_t (static singleton, size=0, align=0).
 *  Supports clone/dispose lifecycle and type_equal/type_extends. */
type_t type_get_void_type(allocator_t allocator);

/** @brief Create a void value (no data, initialized=true). */
struct _value_t;
typedef struct _value_t *value_t;
value_t create_void_value(vm_t vm);

#ifdef __cplusplus
}
#endif
#endif
