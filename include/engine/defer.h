#ifndef _H_CUBEC_ENGINE_DEFER_
#define _H_CUBEC_ENGINE_DEFER_
#include "core/allocator.h"
#include "core/class.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief cfunction_t callback type (shared with func_t).
 *
 * Forward-declared here to avoid circular include with func.h.
 */
struct _vm_t;
typedef value_t (*cfunction_t)(struct _vm_t *vm, value_t fn, size_t argc,
                                value_t *argv);

struct _scope_t;

/**
 * @brief defer_t — deferred execution data object.
 *
 * Mirrors func_t: cfunction_t callback for builtin defers, closure_scope for
 * captured variables, root_scope for lookup chain. For ast_defer_t, func is
 * NULL and execution goes through the AST body instead.
 *
 * Lifecycle: owned by scope->defers (auto_dispose=true vec). Not registered
 * in vm->cfuncs (defer is not a callable value).
 */
struct _defer_t {
  cfunction_t func;               /* C callback (builtin defer), NULL for ast_defer/using */
  struct _scope_t *closure_scope;  /* owned: isolated scope for captured vars */
  struct _scope_t *root_scope;     /* borrowed: vm->root_scope at creation */
  value_t target;                  /* for using defers: borrowed ref to cloned value in closure_scope */
};
typedef struct _defer_t *defer_t;

/** @brief Class descriptor for allocator_create. */
extern class_t g_defer_class;

/** @brief Init args for g_defer_class. */
typedef struct defer_init_t {
  cfunction_t func;
  struct _scope_t *closure_scope; /* nullable, owned (transferred to defer_t) */
  struct _scope_t *root_scope;    /* borrowed: vm->root_scope at creation */
  value_t target;                 /* for using defers: borrowed ref to cloned value */
} defer_init_t;

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_DEFER_ */
