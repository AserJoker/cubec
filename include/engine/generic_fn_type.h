#ifndef _H_CUBEC_ENGINE_GENERIC_FN_TYPE_
#define _H_CUBEC_ENGINE_GENERIC_FN_TYPE_
#include "core/vec.h"
#include "engine/type.h"
#include "engine/generic_param.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generic function type — extends _type_t with a generic parameter table,
 * instance cache, and isolated scope.
 *
 * Compile-time only: vtable.instantiate is the only non-NULL entry.
 * Safe to cast generic_fn_type_t → type_t (base is first field).
 */
struct _generic_fn_type_t {
  struct _type_t base;        /* inherited header */
  vec_t  params;             /* owned (auto_dispose=true): vec of generic_param_t */
  vec_t  instances;          /* owned (auto_dispose=true): vec of generic_instance_t */
  struct _scope_t *scope;    /* owned: isolated scope for instance lifecycle */
  void  *node;               /* borrowed: AST node reference */
  /* value.data holds create_instance_fn_t callback (泛型值是 TYPE_KIND_GENERIC_FN 自身的值) */
};
typedef struct _generic_fn_type_t *generic_fn_type_t;

/** @brief Class descriptor for allocator_create. */
extern class_t g_generic_fn_type_class;

/** @brief Create a generic function type with the given parameter table.
 *  Name is cloned (owned by type_t). node is borrowed (not cloned). */
generic_fn_type_t generic_fn_type_create(allocator_t allocator, const char *name,
                                         vec_t params, void *node);

/* ---- Accessors ---- */

/** @brief Get the generic parameter table (vec of generic_param_t). */
vec_t generic_fn_type_get_params(generic_fn_type_t self);

/** @brief Get the instance cache (vec of generic_instance_t). */
vec_t generic_fn_type_get_instances(generic_fn_type_t self);

/** @brief Get the isolated scope. */
struct _scope_t *generic_fn_type_get_scope(generic_fn_type_t self);

/** @brief Get the borrowed AST node. */
void *generic_fn_type_get_node(generic_fn_type_t self);

#ifdef __cplusplus
}
#endif
#endif
