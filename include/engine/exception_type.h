#ifndef _H_CUBEC_ENGINE_EXCEPTION_TYPE_
#define _H_CUBEC_ENGINE_EXCEPTION_TYPE_
#include "engine/type.h"
#ifdef __cplusplus
extern "C" {
#endif

/** @brief Payload for exception values.
 *  message is an inline buffer allocated with extra space via
 *  allocator_alloc(sizeof(exception_data_t) + msg_len). Using a 1-element
 *  array (instead of a zero-length/flexible array) keeps the struct size
 *  identical under C and C++ and avoids -Wextern-c-compat. */
struct exception_data_t {
  char message[1]; /* owned, zero-terminated */
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
