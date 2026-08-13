#ifndef _H_CUBEC_ENGINE_CALLABLE_TYPE_
#define _H_CUBEC_ENGINE_CALLABLE_TYPE_
#include "engine/type.h"
#include "engine/value.h"
#include "engine/cfunc.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Callable type — extends _type_t with parameter types and return type.
 *
 * Callable values store a cfunc_t object (double pointer, same pattern as str/string_t).
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
} callable_type_init_t;

/* ---- Type creation ---- */

/** @brief Create a callable type: (T1, T2, ...) -> R.
 *  name is auto-generated. size/align are sizeof(cfunc_t)/_Alignof(cfunc_t).
 *  param_types and return_type are deep-copied (owned by callable_type_t). */
callable_type_t callable_type_create(allocator_t allocator, vec_t param_types,
                                      type_t return_type, bool is_variadic,
                                      bool mut);

/* ---- Accessors ---- */

type_t    callable_type_get_param_type(callable_type_t self, uint64_t index);
type_t    callable_type_get_return_type(callable_type_t self);
uint64_t  callable_type_get_param_count(callable_type_t self);
bool      callable_type_is_variadic(callable_type_t self);

/* ---- Value constructors ---- */

struct _vm_t;

/** @brief Create a callable value from a cfunction_t.
 *  Follows the str double-pointer pattern:
 *  value.data = cfunc_t* → cfunc_t (registered in scope->cfuncs). */
value_t create_callable_value(struct _vm_t *vm, callable_type_t ct,
                               cfunction_t func, const char *name);

/** @brief Create a callable shadow value (no cfunc_t). */
value_t create_callable_shadow(struct _vm_t *vm, callable_type_t ct,
                                bool initialized);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_CALLABLE_TYPE_ */
