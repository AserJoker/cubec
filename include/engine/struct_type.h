#ifndef _H_CUBEC_ENGINE_STRUCT_TYPE_
#define _H_CUBEC_ENGINE_STRUCT_TYPE_
#include "engine/type.h"
#include "engine/value.h"
#include "core/strmap.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _scope_t;
typedef struct _scope_t *scope_t;

/* ---- field_info_t ---- */

/**
 * @brief Describes a single instance field of a struct type.
 * Managed via g_field_info_class; stored as pointers in struct_type_t.fields.
 */
struct _field_info_t;
typedef struct _field_info_t *field_info_t;

/** @brief Class descriptor for allocator_create. */
extern class_t g_field_info_class;

/* ---- Accessors ---- */

const char *field_info_get_name(field_info_t self);
type_t      field_info_get_type(field_info_t self);
uint64_t    field_info_get_offset(field_info_t self);
bool        field_info_is_pub(field_info_t self);

/* ---- struct_type_t ---- */

/**
 * @brief Struct type — extends _type_t with fields, scope, props, methods.
 *
 * Safe to cast struct_type_t -> type_t (base is first field).
 * Uses duck typing (structural comparison), name is for reflection/to_string only.
 *
 * NOTE: struct_type_t is an internal type pointer. Public API operates on
 * value_t (TYPE_KIND_TYPE type values). Direct struct_type_t access is only
 * for internal vtable implementations within the engine.
 */
struct _struct_type_t {
  struct _type_t base;       /* inherited header, base.name nullable (anonymous) */
  vec_t  fields;             /* owned (auto_dispose=true): vec of field_info_t* */
  struct _scope_t *scope;    /* owned: manages lifecycle of static props/methods */
  strmap_t props;            /* borrowed (value_auto_dispose=false): name -> value_t */
  strmap_t methods;          /* borrowed (value_auto_dispose=false): name -> value_t */
  strmap_t pub_names;        /* set of names that are pub (name -> dummy value) */
  bool    sealed;            /* true = field layout frozen */
  const char *module_id;     /* borrowed: owning module path or "<builtin>" */
};
typedef struct _struct_type_t *struct_type_t;

/** @brief Class descriptor for allocator_create / alloc_clone. */
extern class_t g_struct_type_class;

/** @brief Init args for g_struct_type_class. */
typedef struct struct_type_init_t {
  type_kind_t kind;
  const char *name;          /* nullable for anonymous struct, will be cloned (owned) */
  uint64_t    size;
  uint64_t    align;
  bool        mut;
  vtable_t    vtable;
  const char *module_id;     /* borrowed: owning module path or "<builtin>" */
} struct_type_init_t;

/* ---- Value-based type operations ---- */

struct _vm_t;

/** @brief Add a field to the struct type value (before seal).
 *  field_type_val must be a TYPE_KIND_TYPE value wrapping the field type.
 *  Returns void value on success, exception value on error (sealed, duplicate name). */
value_t vm_struct_add_field(struct _vm_t *vm, value_t type_val,
                            const char *name, value_t field_type_val, bool pub);

/** @brief Seal the struct type value: finalize field layout.
 *  Returns void value on success, exception value on error. */
value_t vm_struct_seal(struct _vm_t *vm, value_t type_val);

/** @brief Register a static property or method on the struct type value.
 *  is_method=true: registers in both props and methods.
 *  Returns void value on success, exception value on error (duplicate name). */
value_t vm_struct_add_prop(struct _vm_t *vm, value_t type_val,
                           const char *name, value_t val, bool is_method, bool pub);

/* ---- Value-based type accessors ---- */

/** @brief Find a field by name. Returns NULL if not found. */
field_info_t vm_struct_find_field(struct _vm_t *vm, value_t type_val, const char *name);

/** @brief Get the fields vec (field_info_t* elements). */
vec_t vm_struct_get_fields(struct _vm_t *vm, value_t type_val);

/** @brief Get the owned scope. */
scope_t vm_struct_get_scope(struct _vm_t *vm, value_t type_val);

/** @brief Get the props strmap (name -> value_t). */
strmap_t vm_struct_get_props(struct _vm_t *vm, value_t type_val);

/** @brief Get the methods strmap (name -> value_t). */
strmap_t vm_struct_get_methods(struct _vm_t *vm, value_t type_val);

/** @brief Check if the struct type is sealed. */
bool vm_struct_is_sealed(struct _vm_t *vm, value_t type_val);

/** @brief Get the module_id (borrowed). */
const char *vm_struct_get_module_id(struct _vm_t *vm, value_t type_val);

/** @brief Check if a field is pub. */
bool vm_struct_is_field_pub(struct _vm_t *vm, value_t type_val, const char *name);

/** @brief Check if a prop/method is pub. */
bool vm_struct_is_prop_pub(struct _vm_t *vm, value_t type_val, const char *name);

/* ---- Value constructors ---- */

/** @brief Create a struct instance value with given field values.
 *  type_val must be a TYPE_KIND_TYPE value wrapping a sealed struct_type_t.
 *  Each field's data is memcpy'd into a contiguous buffer at the field's offset. */
value_t vm_create_struct_value(struct _vm_t *vm, value_t type_val, value_t *field_values);

/** @brief Create a struct shadow value (no data). */
value_t vm_create_struct_shadow(struct _vm_t *vm, value_t type_val, bool initialized);

/** @brief Create a pointer value pointing to a struct member field.
 *  Returns *FieldType pointer with data = obj.data + field.offset.
 *  Exported for value_member_addr in value.c. */
value_t _struct_value_member_addr(struct _vm_t *vm, value_t self, const char *name);

/* ---- VM convenience ---- */

/** @brief Create a struct type, register in scope->types, wrap as type value.
 *  The struct_type_t is added to current_scope->types (auto-dispose).
 *  Returns the type value (value.data = struct_type_t, own=false). */
value_t vm_create_struct_type_value(struct _vm_t *vm, const char *name,
                                     bool mut, const char *module_id);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_STRUCT_TYPE_ */
