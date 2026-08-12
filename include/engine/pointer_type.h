#ifndef _H_CUBEC_ENGINE_POINTER_TYPE_
#define _H_CUBEC_ENGINE_POINTER_TYPE_
#include "engine/type.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Pointer type — extends _type_t with pointee_type and is_volatile.
 *
 * Pointer values store a raw pointer (void*) to the pointee's data buffer.
 * size = sizeof(void*), align = _Alignof(void*).
 *
 * Grammar: const? * volatile? <type>
 *   - mut=false (const *) → pointer itself is immutable (cannot change target)
 *   - mut=true  (*)       → pointer is mutable
 *   - is_volatile=true    → volatile qualifier (recorded, silently ignored in equals/extends)
 *   - pointee_type        → the type being pointed to (may itself be const)
 *
 * Safe to cast pointer_type_t → type_t (base is first field).
 */
struct _pointer_type_t {
  struct _type_t base;        /* inherited header */
  type_t   pointee_type;     /* owned: the type being pointed to */
  bool     is_volatile;      /* volatile qualifier (silently ignored in equals/extends) */
};
typedef struct _pointer_type_t *pointer_type_t;

/** @brief Class descriptor for allocator_create. */
extern class_t g_pointer_type_class;

/** @brief Init args for g_pointer_type_class. */
typedef struct pointer_type_init_t {
  type_kind_t kind;
  const char *name;
  uint64_t    size;
  uint64_t    align;
  bool        mut;
  vtable_t    vtable;
  type_t      pointee_type;   /* borrowed, will be cloned (owned by pointer_type_t) */
  bool        is_volatile;
} pointer_type_init_t;

/* ---- Type creation ---- */

/** @brief Create a pointer type: *T or const *T or *volatile T etc.
 *  name is auto-generated.
 *  size = sizeof(void*), align = _Alignof(void*).
 *  pointee_type is deep-copied (owned by pointer_type_t). */
pointer_type_t pointer_type_create(allocator_t allocator, type_t pointee_type,
                                    bool mut, bool is_volatile);

/* ---- Accessors ---- */

type_t pointer_type_get_pointee_type(pointer_type_t self);
bool   pointer_type_is_volatile(pointer_type_t self);

/* ---- Value constructors ---- */

struct _vm_t;

/** @brief Create a pointer value pointing to a value's data.
 *  The pointer borrows — does not own the pointee data. */
value_t create_pointer_value(struct _vm_t *vm, pointer_type_t pt, value_t pointee);

/** @brief Create a pointer value from a raw address. */
value_t create_pointer_value_from_addr(struct _vm_t *vm, pointer_type_t pt, void *addr);

/** @brief Create a pointer shadow value (no data). */
value_t create_pointer_shadow(struct _vm_t *vm, pointer_type_t pt, bool initialized);

/* ---- Address-of utility ---- */

/** @brief Get the address of a value as a pointer value.
 *  Returns error for void/type/error kinds (no addressable data). */
value_t value_addrof(struct _vm_t *vm, value_t target);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_POINTER_TYPE_ */
