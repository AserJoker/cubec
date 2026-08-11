#ifndef _H_CUBEC_ENGINE_SLICE_TYPE_
#define _H_CUBEC_ENGINE_SLICE_TYPE_
#include "engine/type.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Slice type — extends _type_t with element_type.
 *
 * Slice values reference a region of an array without owning it.
 * Data layout: struct { void *ptr; uint64_t start; uint64_t len; }
 * size=24, align=8 on 64-bit.
 *
 * Safe to cast slice_type_t → type_t (base is first field).
 */
struct _slice_type_t {
  struct _type_t base;  /* inherited header */
  type_t element_type;  /* borrowed ref to element type */
};
typedef struct _slice_type_t *slice_type_t;

/** @brief Slice value data layout. */
struct slice_data_t {
  void    *ptr;   /* pointer to source array's data buffer */
  uint64_t start; /* byte offset into source buffer */
  uint64_t len;   /* number of elements in this slice */
};

/** @brief Class descriptor for allocator_create. */
extern class_t g_slice_type_class;

/** @brief Init args for g_slice_type_class. */
typedef struct slice_type_init_t {
  type_kind_t kind;
  const char *name;
  uint64_t    size;
  uint64_t    align;
  bool        mut;
  vtable_t    vtable;
  type_t      element_type; /* borrowed ref */
} slice_type_init_t;

/* ---- Type creation ---- */

/** @brief Create a slice type: []element_type.
 *  name is auto-generated as "[]ElemName".
 *  size = sizeof(struct slice_data_t), align = _Alignof(struct slice_data_t). */
slice_type_t slice_type_create(allocator_t allocator, type_t element_type, bool mut);

/* ---- Accessors ---- */

type_t slice_type_get_element_type(slice_type_t self);

/* ---- Value constructors ---- */

struct _vm_t;

/** @brief Create a slice value referencing a region of an array value.
 *  The slice borrows from the array — does not own data.
 *  start_elem and count define the slice range. */
value_t create_slice_value(struct _vm_t *vm, slice_type_t st,
                           value_t array_value, uint64_t start_elem, uint64_t count);

/** @brief Create a slice shadow value (no data). */
value_t create_slice_shadow(struct _vm_t *vm, slice_type_t st, bool initialized);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_SLICE_TYPE_ */
