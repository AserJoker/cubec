#ifndef _H_CUBEC_ENGINE_CFUNC_
#define _H_CUBEC_ENGINE_CFUNC_
#include "core/allocator.h"
#include "core/class.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief C function callback type for callable values.
 * @param vm   Virtual machine context
 * @param fn   The callable value itself (for self-inspection)
 * @param argc Number of arguments
 * @param argv Argument array
 * @return Result value
 */
typedef value_t (*cfunction_t)(struct _vm_t *vm, value_t fn, size_t argc,
                                value_t *argv);

/**
 * @brief cfunc_t — callable value data object (wraps a cfunction_t).
 *
 * Follows the same double-pointer pattern as string_t:
 * value.data = cfunc_t* → cfunc_t → { cfunction_t func }
 * scope->cfuncs owns the cfunc_t; value.own=true frees the cfunc_t* slot.
 */
struct _cfunc_t {
  cfunction_t func;
};
typedef struct _cfunc_t *cfunc_t;

/** @brief Class descriptor for allocator_create. */
extern class_t g_cfunc_class;

/** @brief Init args for g_cfunc_class. */
typedef struct cfunc_init_t {
  cfunction_t func;
} cfunc_init_t;

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_CFUNC_ */
