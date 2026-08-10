#ifndef _H_CUBEC_ENGINE_VALUE_
#define _H_CUBEC_ENGINE_VALUE_
#include "core/allocator.h"
#include "core/class.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

struct _type_t;
typedef struct _type_t *type_t;

/**
 * @brief value_t — opaque pointer to uniform object model value.
 */
struct _value_t;
typedef struct _value_t *value_t;

/** @brief Type descriptor for allocator_create. */
extern class_t g_value_class;

/**
 * @brief Create a value with given type, data, and ownership.
 */
value_t value_create(allocator_t allocator, type_t type, void *data, bool own);

/**
 * @brief Dispose a value. If own=true, calls type->vtable.dispose(data).
 */
void value_dispose(value_t self, allocator_t allocator);

/* ---- Accessors ---- */

type_t  value_get_type(value_t self);
void   *value_get_data(value_t self);
bool    value_is_own(value_t self);
bool    value_is_shadow(value_t self);

/**
 * @brief Value-level equality: delegates to type->vtable.equal.
 * @return bool value on success, error value if vtable.equal is NULL. */
struct _vm_t;
value_t value_equal(struct _vm_t *vm, value_t a, value_t b);

/**
 * @brief Value-level extends: delegates to type->vtable.extends.
 * @return bool value on success, error value if vtable.extends is NULL. */
value_t value_extends(struct _vm_t *vm, value_t sub, value_t super_val);

#ifdef __cplusplus
}
#endif
#endif
