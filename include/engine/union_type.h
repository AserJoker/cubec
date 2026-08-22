#ifndef _H_CUBEC_ENGINE_UNION_TYPE_
#define _H_CUBEC_ENGINE_UNION_TYPE_
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

/* ---- union_data_t: value data layout ---- */

/**
 * @brief Runtime layout of a union value's data buffer.
 * tag = active field index, followed by payload (max field size).
 * Buffer is trivially memcpy-able.
 */
struct union_data_t {
  uint32_t tag;
  /* padding to payload_offset, then payload bytes */
};

/* ---- union_type_t ---- */

/**
 * @brief Tagged union type — extends _type_t with fields, scope, props, methods.
 *
 * Safe to cast union_type_t -> type_t (base is first field).
 * Uses duck typing (structural comparison), name is for reflection/to_string only.
 * All fields share the same payload area; tag indicates which is active.
 *
 * NOTE: union_type_t is an internal type pointer. Public API operates on
 * value_t (TYPE_KIND_TYPE type values). Direct union_type_t access is only
 * for internal vtable implementations within the engine.
 */
struct _union_type_t {
  struct _type_t base;       /* inherited header, base.name nullable (anonymous) */
  vec_t  fields;             /* owned (auto_dispose=true): vec of field_info_t* */
  struct _scope_t *scope;    /* owned: isolated scope for props/methods lifecycle */
  strmap_t props;            /* borrowed (value_auto_dispose=false): name -> value_t */
  strmap_t methods;          /* borrowed (value_auto_dispose=false): name -> value_t */
  strmap_t pub_names;        /* set of names that are pub (name -> dummy value) */
  bool    sealed;            /* true = field layout frozen */
  uint64_t payload_size;     /* max(field sizes) */
  uint64_t payload_offset;   /* offset from data start to payload area */
  const char *module_id;     /* borrowed: owning module path or "<builtin>" */
};
typedef struct _union_type_t *union_type_t;

/** @brief Class descriptor for allocator_create / alloc_clone. */
extern class_t g_union_type_class;

/** @brief Init args for g_union_type_class. */
typedef struct union_type_init_t {
  type_kind_t kind;
  const char *name;          /* nullable for anonymous union, will be cloned (owned) */
  uint64_t    size;
  uint64_t    align;
  bool        mut;
  vtable_t    vtable;
  const char *module_id;     /* borrowed: owning module path or "<builtin>" */
} union_type_init_t;

/* ---- Value-based type operations ---- */

struct _vm_t;

/** @brief Add a variant field to the union type value (before seal).
 *  field_type_val must be a TYPE_KIND_TYPE value wrapping the field type.
 *  The field type is cloned into the union's scope->types.
 *  Returns void value on success, exception value on error (sealed, duplicate name). */
value_t vm_union_add_field(struct _vm_t *vm, value_t type_val,
                           const char *name, value_t field_type_val, bool pub);

/** @brief Seal the union type value: finalize payload_offset and total size.
 *  Returns void value on success, exception value on error. */
value_t vm_union_seal(struct _vm_t *vm, value_t type_val);

/** @brief Register a static property or method on the union type value.
 *  is_method=true: registers in both props and methods.
 *  Returns void value on success, exception value on error (duplicate name). */
value_t vm_union_add_prop(struct _vm_t *vm, value_t type_val,
                          const char *name, value_t val, bool is_method, bool pub);

/* ---- Value-based type accessors ---- */

/** @brief Find a field by name. Returns NULL if not found. */
field_info_t vm_union_find_field(struct _vm_t *vm, value_t type_val, const char *name);

/** @brief Get the fields vec (field_info_t* elements). */
vec_t vm_union_get_fields(struct _vm_t *vm, value_t type_val);

/** @brief Get the owned scope. */
scope_t vm_union_get_scope(struct _vm_t *vm, value_t type_val);

/** @brief Get the props strmap (name -> value_t). */
strmap_t vm_union_get_props(struct _vm_t *vm, value_t type_val);

/** @brief Get the methods strmap (name -> value_t). */
strmap_t vm_union_get_methods(struct _vm_t *vm, value_t type_val);

/** @brief Check if the union type is sealed. */
bool vm_union_is_sealed(struct _vm_t *vm, value_t type_val);

/** @brief Get the module_id (borrowed). */
const char *vm_union_get_module_id(struct _vm_t *vm, value_t type_val);

/** @brief Check if a field is pub. */
bool vm_union_is_field_pub(struct _vm_t *vm, value_t type_val, const char *name);

/** @brief Check if a prop/method is pub. */
bool vm_union_is_prop_pub(struct _vm_t *vm, value_t type_val, const char *name);

/* ---- Value constructors ---- */

/** @brief Create a union instance value with given field name and field value.
 *  type_val must be a TYPE_KIND_TYPE value wrapping a sealed union_type_t.
 *  Looks up field by name, sets tag, memcpy's field data into payload area.
 *  Returns exception value if field not found. */
value_t vm_create_union_value(struct _vm_t *vm, value_t type_val,
                               const char *field_name, value_t field_value);

/** @brief Create a union shadow value (no data). */
value_t vm_create_union_shadow(struct _vm_t *vm, value_t type_val, bool initialized);

/** @brief Create a pointer value pointing to a union member field.
 *  Checks tag matches, returns *FieldType pointer.
 *  Exported for value_member_addr in value.c. */
value_t _union_value_member_addr(struct _vm_t *vm, value_t self, const char *name);

/** @brief Find a field by name from union_type_t directly (read-only).
 *  Internal helper for result_type.c method functions.
 *  Prefer vm_union_find_field for value-based API usage. */
field_info_t _union_type_find_field(union_type_t ut, const char *name);

/** @brief Create a union instance value from union_type_t directly.
 *  Internal helper for result_type.c method functions.
 *  Prefer vm_create_union_value for value-based API usage. */
value_t _union_type_create_value(struct _vm_t *vm, union_type_t ut,
                                  uint32_t tag, value_t field_value);

/* ---- VM convenience ---- */

/** @brief Create a union type value via vm (registered in scope->types).
 *  The union_type_t is added to current_scope->types (auto-dispose).
 *  Returns the type value (value.data = union_type_t, own=false). */
value_t vm_create_union_type_value(struct _vm_t *vm, const char *name,
                                    bool mut, const char *module_id);

/* ---- Generic instantiation ---- */

/** @brief Standard generic instantiation callback for union types.
 *  Checks instance cache, creates a concrete union type on miss by
 *  evaluating the union declaration's field type expressions with
 *  generic parameter substitution.
 *  Follows create_instance_fn_t signature. */
value_t create_union_instance(struct _vm_t *vm, value_t tmpl,
                              size_t argc, value_t *argv);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_UNION_TYPE_ */
