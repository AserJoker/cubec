#ifndef _H_CUBEC_ENGINE_ERROR_TYPE_
#define _H_CUBEC_ENGINE_ERROR_TYPE_
#include "engine/type.h"
#ifdef __cplusplus
extern "C" {
#endif

/** @brief Payload for error values. */
struct error_data_t {
  char *message; /* owned */
};

/** @brief Get the "error" type_t (static singleton). */
type_t type_get_error_type(allocator_t allocator);

/** @brief Create an error value with formatted message.
 *  value.data = error_data_t* (owned). Added to current_scope->values. */
value_t create_error_value(vm_t vm, const char *fmt, ...);

#ifdef __cplusplus
}
#endif
#endif
