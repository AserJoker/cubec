#ifndef _H_CUBEC_ENGINE_ARRAY_TYPE_
#define _H_CUBEC_ENGINE_ARRAY_TYPE_
#include "engine/type.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Array type — extends _type_t with element_type and count.
 *
 * Safe to cast array_type_t → type_t (base is first field).
 */
struct _array_type_t {
  struct _type_t base;  /* inherited header */
  type_t element_type;  /* borrowed ref to element type */
  uint64_t count;       /* array length (compile-time constant) */
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
  uint64_t    count;
} array_type_init_t;

/* ---- Type creation ---- */

/** @brief Create an array type: [count]element_type.
 *  name is auto-generated as "[N]ElemName".
 *  size = count * element_type->size, align = element_type->align. */
array_type_t array_type_create(allocator_t allocator, type_t element_type,
                                uint64_t count, bool mut);

/* ---- Accessors ---- */

type_t    array_type_get_element_type(array_type_t self);
uint64_t  array_type_get_count(array_type_t self);

/* ---- Value constructors ---- */

struct _vm_t;

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
