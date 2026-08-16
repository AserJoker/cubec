#ifndef _H_CUBEC_ENGINE_CUNION_TYPE_
#define _H_CUBEC_ENGINE_CUNION_TYPE_
#include "engine/type.h"
#include "engine/value.h"
#include "core/strmap.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _scope_t;
typedef struct _scope_t *scope_t;

/* ---- field_info_t (defined in struct_type.h) ---- */

struct _field_info_t;
typedef struct _field_info_t *field_info_t;

/* ---- cunion_data_t: value data layout ---- */

/**
 * @brief Runtime layout of a cunion value's data buffer.
 *
 * C-compatible union: a single overlapping memory region of size `base.size`.
 * There is NO tag and NO padding — every field starts at offset 0 and shares
 * the same bytes, exactly like a C `union`. The buffer is trivially memcpy-able.
 */

/* ---- cunion_type_t ---- */

/**
 * @brief C-compatible union type — extends _type_t with fields only.
 *
 * Safe to cast cunion_type_t -> type_t (base is first field).
 * Mirrors a C `union`: all fields overlap at offset 0, the layout size is the
 * maximum field size (aligned), and there are NO methods, NO static properties,
 * NO tag, and NO access control. Field access performs a raw read/write of the
 * shared memory region with no active-variant tracking.
 *
 * NOTE: cunion_type_t is an internal type pointer. Public API operates on
 * value_t (TYPE_KIND_TYPE type values). Direct cunion_type_t access is only
 * for internal vtable implementations within the engine.
 */
struct _cunion_type_t {
  struct _type_t base;       /* inherited header, base.name nullable (anonymous) */
  vec_t  fields;             /* owned (auto_dispose=true): vec of field_info_t* */
  struct _scope_t *scope;    /* owned: isolated scope, manages field type lifecycle */
  bool    sealed;            /* true = field layout frozen */
  const char *module_id;     /* borrowed: owning module path or "<builtin>" */
};
typedef struct _cunion_type_t *cunion_type_t;

/** @brief Class descriptor for allocator_create / alloc_clone. */
extern class_t g_cunion_type_class;

/** @brief Init args for g_cunion_type_class. */
typedef struct cunion_type_init_t {
  type_kind_t kind;
  const char *name;          /* nullable for anonymous cunion, will be cloned (owned) */
  uint64_t    size;
  uint64_t    align;
  bool        mut;
  vtable_t    vtable;
  const char *module_id;     /* borrowed: owning module path or "<builtin>" */
} cunion_type_init_t;

/* ---- Value-based type operations ---- */

struct _vm_t;

/** @brief Add a field to the cunion type value (before seal).
 *  field_type_val must be a TYPE_KIND_TYPE value wrapping the field type.
 *  The field type is cloned into the cunion's scope->types.
 *  Returns void value on success, exception value on error (sealed, duplicate name). */
value_t vm_cunion_add_field(struct _vm_t *vm, value_t type_val,
                            const char *name, value_t field_type_val, bool pub);

/** @brief Seal the cunion type value: finalize size/align.
 *  Returns void value on success, exception value on error. */
value_t vm_cunion_seal(struct _vm_t *vm, value_t type_val);

/* ---- Value-based type accessors ---- */

/** @brief Find a field by name. Returns NULL if not found. */
field_info_t vm_cunion_find_field(struct _vm_t *vm, value_t type_val, const char *name);

/** @brief Get the fields vec (field_info_t* elements). */
vec_t vm_cunion_get_fields(struct _vm_t *vm, value_t type_val);

/** @brief Get the owned scope. */
scope_t vm_cunion_get_scope(struct _vm_t *vm, value_t type_val);

/** @brief Check if the cunion type is sealed. */
bool vm_cunion_is_sealed(struct _vm_t *vm, value_t type_val);

/** @brief Get the module_id (borrowed). */
const char *vm_cunion_get_module_id(struct _vm_t *vm, value_t type_val);

/** @brief Check if a field is pub. */
bool vm_cunion_is_field_pub(struct _vm_t *vm, value_t type_val, const char *name);

/* ---- Value constructors ---- */

/** @brief Create a cunion instance value with given field name and field value.
 *  type_val must be a TYPE_KIND_TYPE value wrapping a sealed cunion_type_t.
 *  Looks up field by name, memcpy's field data into the shared region (offset 0).
 *  Returns exception value if field not found. */
value_t vm_create_cunion_value(struct _vm_t *vm, value_t type_val,
                               const char *field_name, value_t field_value);

/** @brief Create a cunion shadow value (no data). */
value_t vm_create_cunion_shadow(struct _vm_t *vm, value_t type_val, bool initialized);

/** @brief Create a pointer value pointing to a cunion member field.
 *  C-compatible: returns *FieldType pointer to offset 0 (no active-variant check).
 *  Exported for value_member_addr in value.c. */
value_t _cunion_value_member_addr(struct _vm_t *vm, value_t self, const char *name);

/** @brief Find a field by name from cunion_type_t directly (read-only).
 *  Internal helper. Prefer vm_cunion_find_field for value-based API usage. */
field_info_t _cunion_type_find_field(cunion_type_t ct, const char *name);

/** @brief Create a cunion instance value from cunion_type_t directly.
 *  Internal helper. Prefer vm_create_cunion_value for value-based API usage. */
value_t _cunion_type_create_value(struct _vm_t *vm, cunion_type_t ct,
                                  const char *field_name, value_t field_value);

/* ---- VM convenience ---- */

/** @brief Create a cunion type value via vm (registered in scope->types).
 *  The cunion_type_t is added to current_scope->types (auto-dispose).
 *  Returns the type value (value.data = cunion_type_t, own=false). */
value_t vm_create_cunion_type_value(struct _vm_t *vm, const char *name,
                                    bool mut, const char *module_id);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_CUNION_TYPE_ */
