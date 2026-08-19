#ifndef _H_CUBEC_ENGINE_CALLABLE_TYPE_
#define _H_CUBEC_ENGINE_CALLABLE_TYPE_
#include "engine/type.h"
#include "engine/value.h"
#include "engine/func.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callable type — extends _type_t with parameter types and return type.
 *
 * Callable values store a func_t object (borrowed ref), scope->cfuncs owns the lifecycle.
 * The callable type defines the function signature: parameter types + return type + is_variadic.
 *
 * Safe to cast callable_type_t → type_t (base is first field).
 */
struct _callable_type_t {
  struct _type_t base;        /* inherited header */
  vec_t    param_types;       /* owned (auto-dispose=true): vec of type_t */
  type_t   return_type;       /* owned */
  uint64_t param_count;       /* number of fixed parameters */
  bool     is_variadic;       /* true = (T1, T2, ...) -> R */
  const char *module_id;      /* borrowed: owning module path or "<builtin>" */
};
typedef struct _callable_type_t *callable_type_t;

/** @brief Class descriptor for allocator_create. */
extern class_t g_callable_type_class;

/** @brief Init args for g_callable_type_class. */
typedef struct callable_type_init_t {
  type_kind_t kind;
  const char *name;
  uint64_t    size;
  uint64_t    align;
  bool        mut;
  vtable_t    vtable;
  vec_t       param_types;   /* borrowed, will be cloned (owned by callable_type_t) */
  type_t      return_type;   /* borrowed, will be cloned (owned by callable_type_t) */
  uint64_t    param_count;
  bool        is_variadic;
  const char *module_id;     /* borrowed: owning module path or "<builtin>" */
} callable_type_init_t;

/* ---- Type creation ---- */

/** @brief Create a callable type: (T1, T2, ...) -> R.
 *  name is auto-generated. size/align are sizeof(func_t)/_Alignof(func_t).
 *  param_types and return_type are deep-copied (owned by callable_type_t). */
callable_type_t callable_type_create(allocator_t allocator, vec_t param_types,
                                      type_t return_type, bool is_variadic,
                                      bool mut, const char *module_id);

/* ---- Accessors ---- */

type_t    callable_type_get_param_type(callable_type_t self, uint64_t index);
type_t    callable_type_get_return_type(callable_type_t self);
uint64_t  callable_type_get_param_count(callable_type_t self);
bool      callable_type_is_variadic(callable_type_t self);
const char *callable_type_get_module_id(callable_type_t self);

/* ---- Value constructors ---- */

struct _vm_t;

/** @brief Create a callable value from a cfunction_t.
 *  value.data = func_t (borrowed ref, registered in scope->cfuncs). */
value_t create_callable_value(struct _vm_t *vm, callable_type_t ct,
                               cfunction_t func, const char *name);

/** @brief Create a callable shadow value (no func_t). */
value_t create_callable_shadow(struct _vm_t *vm, callable_type_t ct,
                                bool initialized);

/** @brief Capture a variable from the current scope into the callable's closure.
 *
 *  Looks up @p name in the current scope chain, clones the value into the
 *  callable's closure scope, and binds it under the same name.
 *  Creates an isolated closure scope (SCOPE_CLOSURE, parent=NULL) if none
 *  exists yet.
 *
 *  @return The captured value in the closure scope, or exception on error. */
value_t callable_capture(struct _vm_t *vm, value_t callable,
                          const char *name);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_CALLABLE_TYPE_ */
