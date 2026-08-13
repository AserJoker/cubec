#ifndef _H_CUBEC_ENGINE_EXCEPTION_TYPE_
#define _H_CUBEC_ENGINE_EXCEPTION_TYPE_
#include "engine/type.h"
#ifdef __cplusplus
extern "C" {
#endif

/** @brief Payload for exception values.
 *  message is a flexible array member — allocated inline with the struct. */
struct exception_data_t {
  char message[0]; /* owned, zero-terminated */
};

/** @brief Get the "exception" type_t (static singleton). */
type_t type_get_exception_type(allocator_t allocator);

/** @brief Create an exception value with formatted message.
 *  value.data = exception_data_t* (owned). Added to current_scope->values. */
value_t create_exception_value(vm_t vm, const char *fmt, ...);

#ifdef __cplusplus
}
#endif
#endif
