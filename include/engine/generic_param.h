#ifndef _H_CUBEC_ENGINE_GENERIC_PARAM_
#define _H_CUBEC_ENGINE_GENERIC_PARAM_
#include "core/allocator.h"
#include "core/class.h"
#include "core/vec.h"
#include "engine/type.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generic parameter — one entry in a generic type/function's parameter table.
 *
 *  name    — parameter name, e.g. "T" (owned, cloned on init/clone)
 *  type    — parameter type, e.g. "type" for type params (owned, alloc_clone'd)
 *  extends — constraint vec of type_t (owned, auto_dispose=true vec)
 *            e.g. T: Interface1 + Interface2 → [Interface1_type, Interface2_type]
 */
struct _generic_param_t {
  char  *name;     /* owned */
  type_t type;     /* owned (alloc_clone'd) */
  vec_t  extends;  /* owned: auto_dispose=true vec of type_t */
  bool   is_rest;  /* true if this is a variadic pack parameter (...T) */
};
typedef struct _generic_param_t *generic_param_t;

/** @brief Class descriptor for allocator_create. */
extern class_t g_generic_param_class;

/** @brief Init args for g_generic_param_class. */
typedef struct generic_param_init_t {
  const char *name;    /* borrowed, will be cloned */
  type_t      type;    /* borrowed, will be alloc_clone'd */
  vec_t       extends; /* borrowed vec of type_t, each will be alloc_clone'd */
  bool        is_rest; /* true if this is a variadic pack parameter */
} generic_param_init_t;

/** @brief Create a generic parameter. */
generic_param_t generic_param_create(allocator_t allocator, const char *name,
                                     type_t type, vec_t extends, bool is_rest);

/** @brief Accessors. */
const char *generic_param_get_name(generic_param_t self);
type_t      generic_param_get_type(generic_param_t self);
vec_t       generic_param_get_extends(generic_param_t self);
bool        generic_param_is_rest(generic_param_t self);

/**
 * @brief Check whether a type is allowed as a value-type generic parameter.
 * Only basic types (bool, integer, float, str) and enums whose underlying
 * type is a basic type are permitted. Type params (T) are unrestricted.
 */
bool generic_param_is_value_type_allowed(type_t t);

/* ---- create_instance callback ---- */

struct _vm_t;
typedef struct _vm_t *vm_t;
struct _value_t;
typedef struct _value_t *value_t;

/**
 * @brief Callback that instantiates a generic type/function with concrete arguments.
 * Stored in the generic value's data field.
 * @param vm       VM context
 * @param template the generic value itself (for accessing params/instances/scope)
 * @param argc     number of concrete arguments
 * @param argv     concrete argument values
 * @return the instantiated value, or an exception on failure
 */
typedef value_t (*create_instance_fn_t)(vm_t vm, value_t tmpl,
                                        size_t argc, value_t *argv);

/**
 * @brief Create a concrete callable instance from a generic function template.
 * Cache lookup → bind params → eval param/return types → create callable_type +
 * ast_func with template_scope → clone into isolated scope → cache entry.
 */
value_t create_fn_instance(vm_t vm, value_t tmpl, size_t argc, value_t *argv);

/* ---- generic_instance_t — instance cache entry ---- */

/**
 * @brief Generic instance cache entry — one monomorphized instance.
 *  params   — the concrete arguments used to instantiate (owned)
 *  instance — the resulting concrete value (owned)
 */
struct _generic_instance_t {
  vec_t    params;    /* owned: auto_dispose=false vec of value_t (owned by this entry) */
  value_t  instance;  /* owned */
};
typedef struct _generic_instance_t *generic_instance_t;

/** @brief Class descriptor for allocator_create. */
extern class_t g_generic_instance_class;

/** @brief Init args for g_generic_instance_class. */
typedef struct generic_instance_init_t {
  vec_t    params;    /* borrowed vec of value_t, each will be shallow-copied */
  value_t  instance;  /* borrowed, will be shallow-copied */
} generic_instance_init_t;

/** @brief Create a generic instance cache entry. */
generic_instance_t generic_instance_create(allocator_t allocator,
                                           vec_t params, value_t instance);

/** @brief Accessors. */
vec_t   generic_instance_get_params(generic_instance_t self);
value_t generic_instance_get_instance(generic_instance_t self);

#ifdef __cplusplus
}
#endif
#endif
