#ifndef _H_CUBEC_ENGINE_ARRAY_TYPE_
#define _H_CUBEC_ENGINE_ARRAY_TYPE_
#include "engine/type.h"
#include "engine/value.h"
#include "engine/scope.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Array type — extends _type_t with element_type and count.
 *
 * Safe to cast array_type_t → type_t (base is first field).
 *
 * count: compile-time value (i32 for concrete lengths, wildcard for [?]T,
 *        generic_param value for [N]T where N is a type parameter).
 * scope: isolated scope owning the count value's lifecycle.
 */
struct _array_type_t {
  struct _type_t base;  /* inherited header */
  type_t element_type;  /* borrowed ref to element type */
  value_t count;        /* compile-time value: i32 / wildcard / generic_param */
  scope_t scope;        /* isolated scope owning count value */
};
typedef struct _array_type_t *array_type_t;

/** @brief Class descriptor for allocator_create. */
extern class_t g_array_type_class;

/** @brief Init args for g_array_type_class. */
typedef struct array_type_init_t {
  type_kind_t kind;
  const char *name;      /* will be cloned (owned) */
  uint64_t    size;
  uint64_t    align;
  bool        mut;
  vtable_t    vtable;
  type_t      element_type; /* borrowed ref */
  value_t     count;        /* compile-time value (borrowed at init time, cloned into scope) */
} array_type_init_t;

/* ---- Type creation ---- */

struct _vm_t;

/** @brief Create an array type: [count]element_type.
 *  name is auto-generated as "[N]ElemName" or "[?]ElemName".
 *  size = count_value * element_type->size, align = element_type->align.
 *  count must be a value_t of i32 or wildcard type. */
array_type_t array_type_create(struct _vm_t *vm, type_t element_type,
                                value_t count, bool mut);

/* ---- Accessors ---- */

type_t   array_type_get_element_type(array_type_t self);
value_t  array_type_get_count(array_type_t self);

/** @brief Extract the compile-time integer value from count.
 *  Returns 0 for wildcard/generic_param (not yet resolved). */
uint64_t array_type_get_count_value(array_type_t self);

/** @brief Check if count is a wildcard (e.g., [?]T). */
bool array_type_is_count_wildcard(array_type_t self);

/* ---- Value constructors ---- */

/** @brief Create an array value with given elements.
 *  Each element's data is memcpy'd into a contiguous buffer.
 *  Value is registered in vm's current_scope->values. */
value_t create_array_value(struct _vm_t *vm, array_type_t at, value_t *elements);

/** @brief Create an array shadow value (no data).
 *  Value is registered in vm's current_scope->values. */
value_t create_array_shadow(struct _vm_t *vm, array_type_t at, bool initialized);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_ARRAY_TYPE_ */
