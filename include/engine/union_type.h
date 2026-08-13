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

/** @brief Class descriptor for allocator_create. */
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

/* ---- Type creation ---- */

/** @brief Create a union type with given name (nullable).
 *  Initially unsealed: fields can be added via union_type_add_field.
 *  The union's scope is isolated (no parent), fully owned by union_type_t. */
union_type_t union_type_create(allocator_t allocator, const char *name, bool mut,
                                const char *module_id);

/** @brief Add a variant field to the union type (before seal).
 *  All fields share the same payload offset.
 *  Computes payload_size = max(field sizes) incrementally.
 *  Errors if union is already sealed. */
void union_type_add_field(allocator_t allocator, union_type_t ut,
                           const char *name, type_t field_type, bool pub);

/** @brief Seal the union type: finalize payload_offset and total size.
 *  After seal, union_type_add_field will emit an error. */
void union_type_seal(union_type_t ut);

/** @brief Register a static property or method on the union type.
 *  is_method=true: registers in both props and methods.
 *  is_method=false: registers in props only. */
void union_type_add_prop(vm_t vm, union_type_t ut,
                          const char *name, value_t val, bool is_method, bool pub);

/* ---- Accessors ---- */

vec_t    union_type_get_fields(union_type_t self);
scope_t  union_type_get_scope(union_type_t self);
strmap_t union_type_get_props(union_type_t self);
strmap_t union_type_get_methods(union_type_t self);
bool     union_type_is_sealed(union_type_t self);

/** @brief Get the module_id (borrowed) that owns this union type. */
const char *union_type_get_module_id(union_type_t self);

/** @brief Check if a field is pub (accessible across modules). */
bool union_type_is_field_pub(union_type_t self, const char *name);

/** @brief Check if a prop/method is pub (accessible across modules). */
bool union_type_is_prop_pub(union_type_t self, const char *name);

/** @brief Find a field by name. Returns NULL if not found. */
field_info_t union_type_find_field(union_type_t self, const char *name);

/* ---- Value constructors ---- */

struct _vm_t;

/** @brief Create a union value with given tag index and field value.
 *  Sets tag, memcpy's field data into payload area.
 *  Value is registered in vm's current_scope->values. */
value_t create_union_value(struct _vm_t *vm, union_type_t ut,
                            uint32_t tag, value_t field_value);

/** @brief Create a union shadow value (no data).
 *  Value is registered in vm's current_scope->values. */
value_t create_union_shadow(struct _vm_t *vm, union_type_t ut, bool initialized);

/** @brief Create a pointer value pointing to a union member field.
 *  Checks tag matches, returns *FieldType pointer.
 *  Exported for value_member_addr in value.c. */
value_t _union_value_member_addr(struct _vm_t *vm, value_t self, const char *name);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_UNION_TYPE_ */
