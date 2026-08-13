#ifndef _H_CUBEC_ENGINE_ERROR_
#define _H_CUBEC_ENGINE_ERROR_
#include "engine/error_code.h"
#include "engine/type.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _vm_t;

/**
 * @brief Create a user-facing error struct value.
 *
 * Fills: message (up to 127 chars, zero-terminated), error_code,
 *        backtrace (zeroed), backtrace_count (0).
 * Value is registered in vm's current_scope->values.
 */
value_t create_error_value(struct _vm_t *vm, uint64_t error_code, const char *fmt, ...);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_ERROR_ */
