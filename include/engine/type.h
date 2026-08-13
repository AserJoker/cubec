#ifndef _H_CUBEC_ENGINE_TYPE_
#define _H_CUBEC_ENGINE_TYPE_
#include "core/allocator.h"
#include "core/class.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

struct _vm_t;
typedef struct _vm_t *vm_t;

struct _value_t;
typedef struct _value_t *value_t;

/**
 * @brief Type kind — distinguishes value categories.
 */
/** @brief Sentinel value for wildcard count in array types: [?]T. */
#define WILDCARD_COUNT UINT64_MAX

typedef enum type_kind_t {
  TYPE_KIND_TYPE,
  TYPE_KIND_MODULE,
  TYPE_KIND_CALLABLE,
  TYPE_KIND_EXCEPTION,
  /* Primitive */
  TYPE_KIND_VOID,
  TYPE_KIND_BOOL,
  TYPE_KIND_I8, TYPE_KIND_I16, TYPE_KIND_I32, TYPE_KIND_I64,
  TYPE_KIND_U8, TYPE_KIND_U16, TYPE_KIND_U32, TYPE_KIND_U64,
  TYPE_KIND_F16, TYPE_KIND_F32, TYPE_KIND_F64,
  TYPE_KIND_CHAR, TYPE_KIND_STR,
  TYPE_KIND_NIL,
  TYPE_KIND_WILDCARD,
  /* Composite */
  TYPE_KIND_POINTER, TYPE_KIND_ARRAY, TYPE_KIND_SLICE, TYPE_KIND_TUPLE,
  TYPE_KIND_STRUCT, TYPE_KIND_UNION, TYPE_KIND_CUNION,
  TYPE_KIND_ENUM, TYPE_KIND_INTERFACE,
} type_kind_t;

/**
 * @brief type_t — type object (opaque pointer).
 *
 * For TYPE_KIND_TYPE values: value.type and value.data point to the same
 * type_t (type=ref, data=own).
 */
struct _type_t;
typedef struct _type_t *type_t;

/**
 * @brief VTable — type behavior dispatch table.
 *
 *  clone              — value cloning (take vm_t, return value_t)
 *  equal/extends      — value-level computation (take vm_t, return value_t)
 *  type_equal/type_extends — type-level computation (take vm_t, return value_t)
 *
 *  Computation results: bool value on success, error value on failure
 *  (kind mismatch or NULL vtable entry).
 */
struct vtable_t {
  value_t (*clone)        (vm_t vm, value_t self);
  value_t (*equal)        (vm_t vm, value_t a, value_t b);
  value_t (*extends)      (vm_t vm, value_t sub, value_t super_val);
  value_t (*type_equal)   (vm_t vm, type_t a, type_t b);
  value_t (*type_extends) (vm_t vm, type_t sub, type_t super);
  /* Binary operators */
  value_t (*band)         (vm_t vm, value_t a, value_t b); /* & (logical AND) */
  value_t (*bor)          (vm_t vm, value_t a, value_t b); /* | (logical OR) */
  value_t (*bxor)         (vm_t vm, value_t a, value_t b); /* ^ (XOR) */
  /* Arithmetic operators */
  value_t (*add)          (vm_t vm, value_t a, value_t b); /* + */
  value_t (*sub)          (vm_t vm, value_t a, value_t b); /* - */
  value_t (*mul)          (vm_t vm, value_t a, value_t b); /* * */
  value_t (*div)          (vm_t vm, value_t a, value_t b); /* / */
  value_t (*mod)          (vm_t vm, value_t a, value_t b); /* % */
  /* Shift operators */
  value_t (*shl)          (vm_t vm, value_t a, value_t b); /* << */
  value_t (*shr)          (vm_t vm, value_t a, value_t b); /* >> */
  /* Relational operators (ne/ge/le derived from equal/gt/lt in value layer) */
  value_t (*gt)           (vm_t vm, value_t a, value_t b); /* > */
  value_t (*lt)           (vm_t vm, value_t a, value_t b); /* < */
  /* Unary operators */
  value_t (*bnot)         (vm_t vm, value_t a);            /* ~ (bitwise NOT) */
  value_t (*lnot)         (vm_t vm, value_t a);            /* ! (logical NOT) */
  value_t (*pos)          (vm_t vm, value_t a);            /* + (unary plus) */
  value_t (*neg)          (vm_t vm, value_t a);            /* - (unary minus) */
  /* Implicit type conversion */
  value_t (*safe_cast)    (vm_t vm, value_t self, type_t to); /* safe implicit cast */
  /* Assignment */
  value_t (*assignment)   (vm_t vm, value_t lvalue, value_t rvalue); /* = */
  /* String representation */
  value_t (*to_string)    (vm_t vm, value_t self);                   /* toString */
  /* Field access (.field) */
  value_t (*get_field)    (vm_t vm, value_t self, const char *name); /* obj.field */
  value_t (*set_field)    (vm_t vm, value_t self, const char *name, value_t val); /* obj.field = val */
  /* Subscript ([index]) */
  value_t (*get_item)     (vm_t vm, value_t self, value_t index);   /* obj[index] */
  value_t (*set_item)     (vm_t vm, value_t self, value_t index, value_t val); /* obj[index] = val */
  /* Dereference (*ptr) */
  value_t (*deref_get)    (vm_t vm, value_t self);                  /* *ptr */
  value_t (*deref_set)    (vm_t vm, value_t self, value_t val);     /* *ptr = val */
  /* Slicing (value[start..start+count]) */
  value_t (*slice)        (vm_t vm, value_t self, uint64_t start, uint64_t count);
  /* Function call */
  value_t (*call)         (vm_t vm, value_t self, size_t argc, value_t *argv);
  /* Member call (a.method(args) — method dispatch with implicit self) */
  value_t (*member_call)  (vm_t vm, value_t self, const char *name,
                           size_t argc, value_t *argv);
  /* Static property access (Type::prop) — only TYPE_KIND_TYPE implements get_prop/set_prop;
   * it delegates to the inner type's type_get_prop/type_set_prop. */
  value_t (*get_prop)     (vm_t vm, value_t self, const char *name);
  value_t (*set_prop)     (vm_t vm, value_t self, const char *name, value_t val);
  /* Type-level property access — each type implements its own */
  value_t (*type_get_prop)(vm_t vm, type_t self, const char *name);
  value_t (*type_set_prop)(vm_t vm, type_t self, const char *name, value_t val);
  /* Instance type check: value is Type — checks if the active variant matches the given type */
  value_t (*is_instance) (vm_t vm, value_t self, type_t type);
  /* Raw field read — bypasses result wrapping for path-narrowed access (e.g. after `if u is T`).
   * Only union types implement this; returns the field value directly without result[T,error].
   * NULL for types that do not support raw field access. */
  value_t (*get_field_raw)(vm_t vm, value_t self, const char *name);
};
typedef struct vtable_t vtable_t;

/**
 * @brief type_t — type object (opaque pointer).
 *
 * For TYPE_KIND_TYPE values: value.type and value.data point to the same
 * type_t (type=ref, data=own).
 */
struct _type_t {
  type_kind_t kind;
  char *name;
  uint64_t    size;
  uint64_t    align;
  bool        mut;   /* true = mutable, false = read-only */
  vtable_t    vtable;
};

/* ---- Accessors ---- */

type_kind_t type_get_kind(type_t self);
const char *type_get_name(type_t self);
uint64_t    type_get_size(type_t self);
uint64_t    type_get_align(type_t self);
bool        type_is_mut(type_t self);
bool        type_is_mut(type_t self);
vtable_t    type_get_vtable(type_t self);

/* ---- Class descriptor ---- */

/** @brief Type descriptor for allocator_create.
 *  Dynamic types (array, slice, etc.) should be created via
 *  allocator_create(allocator, &g_type_class, &init). */
extern class_t g_type_class;

/** @brief Init args for g_type_class. */
typedef struct type_init_t {
  type_kind_t kind;
  const char *name;   /* will be cloned (owned by type_t) */
  uint64_t    size;
  uint64_t    align;
  bool        mut;
  vtable_t    vtable;
} type_init_t;

/** @brief Create a dynamic type_t via allocator_create.
 *  Convenience wrapper around allocator_create(&g_type_class, &init). */
type_t type_create(allocator_t allocator, type_kind_t kind, const char *name,
                   uint64_t size, uint64_t align, bool mut, vtable_t vtable);

/* ---- Bootstrap ---- */

/** @brief Get the self-referential "type" type_t (static singleton).
 *  kind=TYPE_KIND_TYPE, name="type", with clone/dispose vtable. */
type_t type_get_type_type(allocator_t allocator);

/** @brief Create a type value wrapping the given type_t.
 *
 *  value.type = vm's "type" type_t (ref), value.data = type.
 *  own=true for heap-allocated types, own=false for static singletons.
 *  If name is non-NULL, creates a name entry in vm's current scope.
 *  The value is added to vm's current_scope->values.
 */
value_t create_type_value(vm_t vm, type_t type, const char *name, bool own);

#ifdef __cplusplus
}
#endif
#endif
