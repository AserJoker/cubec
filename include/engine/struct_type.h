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

/** @brief Class descriptor for allocator_create. */
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

/* ---- Type creation ---- */

/** @brief Create a struct type with given name (nullable).
 *  Initially unsealed: fields can be added via struct_type_add_field.
 *  The struct's scope is isolated (no parent), fully owned by struct_type_t. */
struct_type_t struct_type_create(allocator_t allocator, const char *name,
                                  bool mut, const char *module_id);

/** @brief Add a field to the struct type (before seal).
 *  Computes incremental offset using C alignment rules.
 *  Calls allocator_create for field_info_t via g_field_info_class.
 *  Errors if struct is already sealed. */
void struct_type_add_field(allocator_t allocator, struct_type_t st,
                           const char *name, type_t field_type, bool pub);

/** @brief Seal the struct type: finalize total size with trailing padding.
 *  Computes base.size = align_up(current_offset, base.align).
 *  After seal, struct_type_add_field will emit an error. */
bool struct_type_seal(struct_type_t st);

/** @brief Register a static property or method on the struct type.
 *  is_method=true: registers in both props and methods.
 *  is_method=false: registers in props only.
 *  The value is added to the struct's owned scope for lifecycle management. */
void struct_type_add_prop(vm_t vm, struct_type_t st,
                          const char *name, value_t val, bool is_method, bool pub);

/* ---- Accessors ---- */

vec_t    struct_type_get_fields(struct_type_t self);
scope_t  struct_type_get_scope(struct_type_t self);
strmap_t struct_type_get_props(struct_type_t self);
strmap_t struct_type_get_methods(struct_type_t self);
bool     struct_type_is_sealed(struct_type_t self);

/** @brief Find a field by name. Returns NULL if not found. */
field_info_t struct_type_find_field(struct_type_t self, const char *name);

/** @brief Get the module_id (borrowed) that owns this struct type. */
const char *struct_type_get_module_id(struct_type_t self);

/** @brief Check if a field is pub (accessible across modules). */
bool struct_type_is_field_pub(struct_type_t self, const char *name);

/** @brief Check if a prop/method is pub (accessible across modules). */
bool struct_type_is_prop_pub(struct_type_t self, const char *name);

/* ---- Value constructors ---- */

struct _vm_t;

/** @brief Create a struct value with given field values.
 *  Each field's data is memcpy'd into a contiguous buffer at the field's offset.
 *  Value is registered in vm's current_scope->values. */
value_t create_struct_value(struct _vm_t *vm, struct_type_t st, value_t *field_values);

/** @brief Create a struct shadow value (no data).
 *  Value is registered in vm's current_scope->values. */
value_t create_struct_shadow(struct _vm_t *vm, struct_type_t st, bool initialized);

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
