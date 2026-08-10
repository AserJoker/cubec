#ifndef _H_CUBEC_ENGINE_TYPE_
#define _H_CUBEC_ENGINE_TYPE_
#include "core/allocator.h"
#include "core/class.h"
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
typedef enum type_kind_t {
  TYPE_KIND_TYPE,
  TYPE_KIND_MODULE,
  TYPE_KIND_FUNCTION,
  TYPE_KIND_CALLBACK,
  TYPE_KIND_ERROR,
  /* Primitive */
  TYPE_KIND_VOID,
  TYPE_KIND_BOOL,
  TYPE_KIND_I8, TYPE_KIND_I16, TYPE_KIND_I32, TYPE_KIND_I64,
  TYPE_KIND_U8, TYPE_KIND_U16, TYPE_KIND_U32, TYPE_KIND_U64,
  TYPE_KIND_F16, TYPE_KIND_F32, TYPE_KIND_F64,
  TYPE_KIND_CHAR, TYPE_KIND_STR,
  TYPE_KIND_NIL,
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
 *  clone/dispose — lifecycle
 *  equal/extends — value-level computation (delegate to type_* for type values)
 *  type_equal/type_extends — type-level computation
 */
struct vtable_t {
  value_t (*clone)        (allocator_t alloc, value_t obj);
  void     (*dispose)     (allocator_t alloc, value_t obj);
  bool     (*equal)       (value_t a, value_t b);
  bool     (*extends)     (value_t sub, value_t super_val);
  bool     (*type_equal)  (type_t a, type_t b);
  bool     (*type_extends)(type_t sub, type_t super);
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
  const char *name;
  uint64_t    size;
  uint64_t    align;
  vtable_t    vtable;
};

/* ---- Accessors ---- */

type_kind_t type_get_kind(type_t self);
const char *type_get_name(type_t self);
uint64_t    type_get_size(type_t self);
uint64_t    type_get_align(type_t self);
vtable_t    type_get_vtable(type_t self);

/* ---- Bootstrap ---- */

/** @brief Create the self-referential "type" type_t.
 *  kind=TYPE_KIND_TYPE, name="type", with clone/dispose vtable. */
type_t type_create_type_type(allocator_t allocator);

/** @brief Create a type value wrapping the given type_t.
 *
 *  value.type = vm's "type" type_t (ref), value.data = type (own).
 *  If name is non-NULL, creates a NAME_TYPE entry in vm's current scope.
 *  The value is added to vm's current_scope->values.
 */
value_t create_type_value(vm_t vm, type_t type, const char *name);

#ifdef __cplusplus
}
#endif
#endif
