#ifndef _H_CUBEC_ENGINE_INTERFACE_TYPE_
#define _H_CUBEC_ENGINE_INTERFACE_TYPE_
#include "engine/type.h"
#include "engine/value.h"
#include "engine/callable_type.h"
#include "core/strmap.h"
#ifdef __cplusplus
extern "C" {
#endif

/* ---- interface_type_t ---- */

/**
 * @brief Interface type — compile-time constraint type for extends expressions.
 *
 * An interface describes a set of method signatures (name -> callable_type_t).
 * It has no instances, no fields, no data layout — it exists only as the
 * right-hand side of `type_extends` to check whether a concrete type (struct/union)
 * implements all required methods with compatible signatures.
 *
 * Safe to cast interface_type_t -> type_t (base is first field).
 * Uses duck typing (structural comparison), name is for reflection/to_string only.
 *
 * NOTE: interface_type_t is an internal type pointer. Public API operates on
 * value_t (TYPE_KIND_TYPE type values). Direct interface_type_t access is only
 * for internal vtable implementations within the engine.
 */
struct _interface_type_t {
  struct _type_t base;       /* inherited header, base.name nullable (anonymous) */
  strmap_t methods;          /* owned: name -> callable_type_t (value_auto_dispose=false) */
  bool    sealed;            /* true = method set frozen */
  const char *module_id;     /* borrowed: owning module path or "<builtin>" */
};
typedef struct _interface_type_t *interface_type_t;

/** @brief Class descriptor for allocator_create / alloc_clone. */
extern class_t g_interface_type_class;

/** @brief Init args for g_interface_type_class. */
typedef struct interface_type_init_t {
  type_kind_t kind;
  const char *name;          /* will be cloned (owned) */
  bool        mut;
  vtable_t    vtable;
  const char *module_id;     /* borrowed: owning module path or "<builtin>" */
} interface_type_init_t;

/* ---- Value-based type operations ---- */

struct _vm_t;

/** @brief Add a method signature to the interface type value (before seal).
 *  The callable_type_t is deep-cloned and owned by the interface.
 *  Returns NULL on success, exception value on error (sealed, duplicate name). */
value_t vm_interface_add_method(struct _vm_t *vm, value_t type_val,
                                 const char *name, value_t callable_type_val);

/** @brief Seal the interface type value: freeze method set.
 *  Returns NULL on success, exception value on error (empty interface). */
value_t vm_interface_seal(struct _vm_t *vm, value_t type_val);

/* ---- Value-based type accessors ---- */

/** @brief Get the methods strmap (name -> callable_type_t). */
strmap_t vm_interface_get_methods(struct _vm_t *vm, value_t type_val);

/** @brief Check if the interface is sealed. */
bool vm_interface_is_sealed(struct _vm_t *vm, value_t type_val);

/** @brief Find a method signature by name. Returns NULL if not found. */
callable_type_t vm_interface_find_method(struct _vm_t *vm, value_t type_val, const char *name);

/** @brief Get the module_id (borrowed) that owns this interface type. */
const char *vm_interface_get_module_id(struct _vm_t *vm, value_t type_val);

/** @brief Check whether sub_methods implements all methods of the interface.
 *  Called by struct_type_extends and union_type_extends when super is INTERFACE.
 *  sub_methods is the concrete type's methods strmap (name -> callable value_t).
 *  Returns bool value: true if every interface method is satisfied. */
value_t vm_interface_check_extends(struct _vm_t *vm, value_t type_val,
                                    strmap_t sub_methods);

/** @brief Check extends from interface_type_t directly (read-only).
 *  Internal helper for struct/union type_extends vtable functions.
 *  Prefer vm_interface_check_extends for value-based API usage. */
value_t _interface_type_check_extends(struct _vm_t *vm, interface_type_t it,
                                       strmap_t sub_methods);

/* ---- VM convenience ---- */

/** @brief Create an interface type value via vm (registered in scope->types).
 *  The interface_type_t is added to current_scope->types (auto-dispose).
 *  Returns the type value (value.data = interface_type_t, own=false). */
value_t vm_create_interface_type_value(struct _vm_t *vm, const char *name,
                                        bool mut, const char *module_id);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_INTERFACE_TYPE_ */
