#ifndef _H_CUBEC_ENGINE_ENUM_TYPE_
#define _H_CUBEC_ENGINE_ENUM_TYPE_
#include "engine/type.h"
#include "engine/value.h"
#include "core/strmap.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _scope_t;
typedef struct _scope_t *scope_t;

/**
 * @brief Enum type — extends _type_t with an underlying type and named items.
 *
 * TypeScript-style enum: bound to a single underlying type (primitive, struct,
 * array, or any type supporting ==). Each item is a compile-time-known value of
 * the underlying type. Enum values (variables, items) OWN a real memory buffer
 * holding the underlying value (not a borrowed reference).
 *
 * Safe to cast enum_type_t -> type_t (base is first field).
 *
 * Items live in an isolated scope (no parent) owned by enum_type_t, and are
 * indexed by name in `items` (borrowed refs to value_t in scope->values).
 * Access via `Color::Red` goes through type_get_prop.
 */
struct _enum_type_t {
  struct _type_t base;       /* inherited header, kind = TYPE_KIND_ENUM */
  type_t underlying;          /* owned: the bound underlying type (alloc_clone) */
  struct _scope_t *scope;     /* owned: isolated scope holding item values */
  strmap_t items;             /* borrowed: name -> value_t (item values) */
  const char *module_id;      /* borrowed: owning module path or "<builtin>" */
};
typedef struct _enum_type_t *enum_type_t;

/** @brief Class descriptor for allocator_create. */
extern class_t g_enum_type_class;

/** @brief Init args for g_enum_type_class. */
typedef struct enum_type_init_t {
  type_kind_t kind;
  const char *name;          /* nullable for anonymous enum, will be cloned (owned) */
  uint64_t    size;
  uint64_t    align;
  bool        mut;
  vtable_t    vtable;
  type_t      underlying;     /* borrowed, will be cloned (owned by enum_type_t) */
  const char *module_id;      /* borrowed */
} enum_type_init_t;

/* ---- Type creation (internal engine use) ---- */

/** @brief Create an enum type bound to underlying.
 *  underlying is deep-copied (owned by enum_type_t). */
enum_type_t enum_type_create(allocator_t allocator, const char *name,
                             type_t underlying, bool mut,
                             const char *module_id);

/* ---- Accessors ---- */

type_t    enum_type_get_underlying(enum_type_t self);
scope_t   enum_type_get_scope(enum_type_t self);
const char *enum_type_get_module_id(enum_type_t self);

/** @brief Add a named item with an owned underlying-value buffer.
 *  item_data must be a buffer of underlying size; it is memcpy'd into a fresh
 *  owned buffer. Returns exception value on duplicate name. */
value_t enum_type_add_item(struct _vm_t *vm, enum_type_t self,
                           const char *name, const void *item_data);

/** @brief Look up an item value by name. Returns NULL if not found. */
value_t enum_type_find_item(enum_type_t self, const char *name);

/* ---- Value constructors ---- */

struct _vm_t;

/** @brief Create an enum value owning a real underlying buffer (zeroed). */
value_t create_enum_value(struct _vm_t *vm, enum_type_t et, const void *data);

/** @brief Create an enum shadow value (no data). */
value_t create_enum_shadow(struct _vm_t *vm, enum_type_t et, bool initialized);

/* ---- VM convenience ---- */

/** @brief Create an enum type, register in scope->types, wrap as type value.
 *  The enum_type_t is added to current_scope->types (auto-dispose).
 *  underlying_type_val must be a TYPE_KIND_TYPE value wrapping the underlying type.
 *  Returns the type value (value.data = enum_type_t, own=false). */
value_t vm_create_enum_type_value(struct _vm_t *vm, const char *name,
                                  value_t underlying_type_val, bool mut,
                                  const char *module_id);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_ENUM_TYPE_ */
