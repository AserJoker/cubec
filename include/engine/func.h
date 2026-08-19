#ifndef _H_CUBEC_ENGINE_FUNC_
#define _H_CUBEC_ENGINE_FUNC_
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
 * @brief func_t — callable value data object (wraps a cfunction_t).
 *
 * value.data = func_t (borrowed ref), scope->cfuncs owns the lifecycle.
 *
 * closure_scope: owned isolated scope for captured variables. NULL for plain
 * C functions; set when the callable captures its lexical environment (closures,
 * AST functions, partial applications).
 *
 * root_scope: borrowed reference to vm->root_scope at creation time.
 * Used as the parent of closure_scope for lookup chain.
 */
struct _func_t {
  cfunction_t func;
  const char *name;  /* nullable: function name for call stack / debugging */
  struct _scope_t *closure_scope; /* owned: isolated scope for captured vars */
  struct _scope_t *root_scope;    /* borrowed: vm->root_scope at creation time */
};
typedef struct _func_t *func_t;

/** @brief Class descriptor for allocator_create. */
extern class_t g_func_class;

/** @brief Init args for g_func_class. */
typedef struct func_init_t {
  cfunction_t func;
  const char *name;  /* nullable, borrowed reference (not cloned/freed) */
  struct _scope_t *closure_scope; /* nullable, owned (transferred to func_t) */
  struct _scope_t *root_scope;    /* borrowed: vm->root_scope at creation */
} func_init_t;

/** @brief Accessors */
struct _scope_t *func_get_closure_scope(func_t self);
void            func_set_closure_scope(func_t self, struct _scope_t *scope);
struct _scope_t *func_get_root_scope(func_t self);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_FUNC_ */
