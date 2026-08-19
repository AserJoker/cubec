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

struct _scope_t;

/**
 * @brief cfunc_t — callable value data object (wraps a cfunction_t).
 *
 * Follows the same double-pointer pattern as string_t:
 * value.data = cfunc_t* → cfunc_t → { cfunction_t func }
 * scope->cfuncs owns the cfunc_t; value.own=true frees the cfunc_t* slot.
 *
 * closure_scope: owned isolated scope for captured variables. NULL for plain
 * C functions; set when the callable captures its lexical environment (closures,
 * AST functions, partial applications). On call, the closure_scope is pushed
 * before invoking func, so captured names are visible during execution.
 */
struct _cfunc_t {
  cfunction_t func;
  const char *name;  /* nullable: function name for call stack / debugging */
  struct _scope_t *closure_scope; /* owned: isolated scope for captured vars */
};
typedef struct _cfunc_t *cfunc_t;

/** @brief Class descriptor for allocator_create. */
extern class_t g_cfunc_class;

/** @brief Init args for g_cfunc_class. */
typedef struct cfunc_init_t {
  cfunction_t func;
  const char *name;  /* nullable, borrowed reference (not cloned/freed) */
  struct _scope_t *closure_scope; /* nullable, owned (transferred to cfunc_t) */
} cfunc_init_t;

/** @brief Accessors */
struct _scope_t *cfunc_get_closure_scope(cfunc_t self);
void            cfunc_set_closure_scope(cfunc_t self, struct _scope_t *scope);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_CFUNC_ */
