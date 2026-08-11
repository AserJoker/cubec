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
bool    value_is_initialized(value_t self);
void    value_set_initialized(value_t self, bool initialized);

/**
 * @brief Value-level equality: delegates to type->vtable.equal.
 * @return bool value on success, error value if vtable.equal is NULL. */
struct _vm_t;
value_t value_equal(struct _vm_t *vm, value_t a, value_t b);

/**
 * @brief Value-level extends: delegates to type->vtable.extends.
 * @return bool value on success, error value if vtable.extends is NULL. */
value_t value_extends(struct _vm_t *vm, value_t sub, value_t super_val);

/* ---- Binary operators ---- */

/** @brief & (logical AND): delegates to type->vtable.band. */
value_t value_band(struct _vm_t *vm, value_t a, value_t b);

/** @brief | (logical OR): delegates to type->vtable.bor. */
value_t value_bor(struct _vm_t *vm, value_t a, value_t b);

/** @brief ^ (XOR): delegates to type->vtable.bxor. */
value_t value_bxor(struct _vm_t *vm, value_t a, value_t b);

/* ---- Unary operators ---- */

/** @brief ~ (bitwise NOT): delegates to type->vtable.bnot. */
value_t value_bnot(struct _vm_t *vm, value_t a);

/** @brief ! (logical NOT): delegates to type->vtable.lnot. */
value_t value_lnot(struct _vm_t *vm, value_t a);

/** @brief + (unary plus): delegates to type->vtable.pos. */
value_t value_pos(struct _vm_t *vm, value_t a);

/** @brief - (unary minus): delegates to type->vtable.neg. */
value_t value_neg(struct _vm_t *vm, value_t a);

/* ---- Arithmetic operators ---- */

/** @brief + : delegates to type->vtable.add. */
value_t value_add(struct _vm_t *vm, value_t a, value_t b);

/** @brief - : delegates to type->vtable.sub. */
value_t value_sub(struct _vm_t *vm, value_t a, value_t b);

/** @brief * : delegates to type->vtable.mul. */
value_t value_mul(struct _vm_t *vm, value_t a, value_t b);

/** @brief / : delegates to type->vtable.div. */
value_t value_div(struct _vm_t *vm, value_t a, value_t b);

/** @brief % : delegates to type->vtable.mod. */
value_t value_mod(struct _vm_t *vm, value_t a, value_t b);

/* ---- Shift operators ---- */

/** @brief << : delegates to type->vtable.shl. */
value_t value_shl(struct _vm_t *vm, value_t a, value_t b);

/** @brief >> : delegates to type->vtable.shr. */
value_t value_shr(struct _vm_t *vm, value_t a, value_t b);

/* ---- Relational operators ---- */

/** @brief > : delegates to type->vtable.gt. */
value_t value_gt(struct _vm_t *vm, value_t a, value_t b);

/** @brief < : delegates to type->vtable.lt. */
value_t value_lt(struct _vm_t *vm, value_t a, value_t b);

/** @brief != : delegates to type->vtable.ne. */
value_t value_ne(struct _vm_t *vm, value_t a, value_t b);

/** @brief >= : delegates to type->vtable.ge. */
value_t value_ge(struct _vm_t *vm, value_t a, value_t b);

/** @brief <= : delegates to type->vtable.le. */
value_t value_le(struct _vm_t *vm, value_t a, value_t b);

/** @brief Safely implicitly cast a value to target type.
 *  Delegates to type->vtable.safe_cast. Returns casted value or error. */
value_t value_safe_cast(struct _vm_t *vm, value_t val, type_t to);

/** @brief Assign rvalue to lvalue. Delegates to type->vtable.assignment.
 *  Checks TDZ/const rules, then copies data and sets initialized=true. */
value_t value_assignment(struct _vm_t *vm, value_t lvalue, value_t rvalue);

#ifdef __cplusplus
}
#endif
#endif
