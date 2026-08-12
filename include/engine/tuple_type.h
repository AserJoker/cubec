#ifndef _H_CUBEC_ENGINE_TUPLE_TYPE_
#define _H_CUBEC_ENGINE_TUPLE_TYPE_
#include "engine/type.h"
#include "engine/value.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Tuple type — extends _type_t with element_types vec.
 *
 * Tuple values store heterogeneous elements in a contiguous buffer
 * with per-element offsets computed from alignment.
 * size = total byte size of all fields (with padding), align = max alignment.
 *
 * Safe to cast tuple_type_t → type_t (base is first field).
 */
struct _tuple_type_t {
  struct _type_t base;        /* inherited header */
  vec_t element_types;        /* vec_t of borrowed type_t refs */
  uint64_t *offsets;          /* byte offset of each element (owned) */
  uint64_t field_count;       /* number of elements */
};
typedef struct _tuple_type_t *tuple_type_t;

/** @brief Class descriptor for allocator_create. */
extern class_t g_tuple_type_class;

/** @brief Init args for g_tuple_type_class. */
typedef struct tuple_type_init_t {
  type_kind_t kind;
  const char *name;
  uint64_t    size;
  uint64_t    align;
  bool        mut;
  vtable_t    vtable;
  vec_t       element_types; /* borrowed ref */
  uint64_t   *offsets;       /* will be cloned (owned by tuple_type_t) */
  uint64_t    field_count;
} tuple_type_init_t;

/* ---- Type creation ---- */

/** @brief Create a tuple type: <T1, T2, ...>.
 *  name is auto-generated as "<T1, T2, ...>".
 *  size and align are computed from element types with alignment padding.
 *  offsets[i] gives the byte offset of element i in the data buffer. */
tuple_type_t tuple_type_create(allocator_t allocator, vec_t element_types,
                                bool mut);

/* ---- Accessors ---- */

type_t    tuple_type_get_element_type(tuple_type_t self, uint64_t index);
uint64_t  tuple_type_get_field_count(tuple_type_t self);
uint64_t  tuple_type_get_offset(tuple_type_t self, uint64_t index);

/* ---- Value constructors ---- */

struct _vm_t;

/** @brief Create a tuple value from an array of element values.
 *  elements[i] must have type matching element_types[i].
 *  Data is packed into a contiguous buffer with alignment padding. */
value_t create_tuple_value(struct _vm_t *vm, tuple_type_t tt,
                            value_t *elements);

/** @brief Create a tuple shadow value (no data). */
value_t create_tuple_shadow(struct _vm_t *vm, tuple_type_t tt,
                             bool initialized);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_TUPLE_TYPE_ */
