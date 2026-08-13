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
 * @brief Clone a value: delegates to type->vtable.clone.
 * @return cloned value on success, error value if vtable.clone is NULL. */
value_t value_clone(struct _vm_t *vm, value_t self);

/**
 * @brief Clone a type into vm's current scope via alloc_clone.
 *  Uses the type's class_t.clone to deep-copy, then registers in scope->types.
 * @return cloned type_t registered in current scope. */
type_t value_type_clone(struct _vm_t *vm, type_t self);

/**
 * @brief Dereference: delegates to type->vtable.deref_get.
 *  Reads the value pointed to by a pointer/reference.
 * @return pointed-to value, or error if vtable.deref_get is NULL. */
value_t value_deref_get(struct _vm_t *vm, value_t self);

/**
 * @brief Dereference assignment: delegates to type->vtable.deref_set.
 *  Writes a value through a pointer/reference.
 * @return void value on success, error if vtable.deref_set is NULL. */
value_t value_deref_set(struct _vm_t *vm, value_t self, value_t val);

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

/** @brief Convert value to its string representation.
 *  Delegates to type->vtable.to_string. Returns str value or error. */
value_t value_to_string(struct _vm_t *vm, value_t self);

/** @brief Read a named field: obj.name. Delegates to type->vtable.get_field. */
value_t value_get_field(struct _vm_t *vm, value_t self, const char *name);

/** @brief Write a named field: obj.name = val. Delegates to type->vtable.set_field. */
value_t value_set_field(struct _vm_t *vm, value_t self, const char *name, value_t val);

/** @brief Read an item by index: obj[index]. Delegates to type->vtable.get_item. */
value_t value_get_item(struct _vm_t *vm, value_t self, value_t index);

/** @brief Write an item by index: obj[index] = val. Delegates to type->vtable.set_item. */
value_t value_set_item(struct _vm_t *vm, value_t self, value_t index, value_t val);

/** @brief Slice a value: value[start..start+count]. Delegates to type->vtable.slice.
 *  For arrays: returns a slice value referencing the array's data.
 *  For slices: returns a new slice value referencing the same underlying data.
 *  For str: returns a new str value with the substring. */
value_t value_slice(struct _vm_t *vm, value_t self, uint64_t start, uint64_t count);

/** @brief Call a callable value: fn(argc, argv). Delegates to type->vtable.call.
 *  Performs argument safe_cast to declared param types and return value safe_cast.
 * @return result value on success, error if type is not callable or call fails. */
value_t value_call(struct _vm_t *vm, value_t fn, size_t argc, value_t *argv);

/** @brief Take the address of a value, returning a pointer value.
 *  Returns error for void/type/error kinds (no addressable data). */
struct _pointer_type_t;
value_t value_addrof(struct _vm_t *vm, value_t target);

/** @brief Get pointer to a struct member field: &obj.field.
 *  Returns a pointer value pointing to obj.data + field.offset. */
value_t value_member_addr(struct _vm_t *vm, value_t self, const char *name);

/** @brief Member call: obj.method(args). Delegates to type->vtable.member_call.
 *  For structs: looks up method, passes addrof(self) as first arg. */
value_t value_member_call(struct _vm_t *vm, value_t self, const char *name,
                          size_t argc, value_t *argv);

/** @brief Read a static property: Type::prop. Delegates to type->vtable.get_prop. */
value_t value_get_prop(struct _vm_t *vm, value_t self, const char *name);

/** @brief Write a static property: Type::prop = val. Delegates to type->vtable.set_prop. */
value_t value_set_prop(struct _vm_t *vm, value_t self, const char *name, value_t val);

/** @brief Check if value's active variant matches the given type: value is Type.
 *  For unions: checks if the active tag's field type equals the given type.
 *  For pointers: auto-derefs then delegates.
 *  Returns bool value, or shadow bool for shadow values. */
value_t value_is(struct _vm_t *vm, value_t self, type_t type);

#ifdef __cplusplus
}
#endif
#endif
