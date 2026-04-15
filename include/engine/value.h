#ifndef _H_CUBEC_ENGINE_VALUE_
#define _H_CUBEC_ENGINE_VALUE_
#include "core/allocator.h"
#include "engine/type.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _value_t *value_t;
struct _context_t;
value_t create_value(allocator_t allocator, type_t type, bool mutable,
                     const void *data);
void value_set_comptime(value_t self, bool comptime);
bool value_is_comptime(value_t self);
type_t value_get_type(value_t value);
bool value_type_is(value_t value, type_kind_t kind);
bool value_is_mutable(value_t value);
void value_set_mutable(value_t value, bool mutable);
void *value_get_data(value_t value);
value_t value_clone(allocator_t allocator, value_t value);
value_t value_assigment(value_t self, struct _context_t *ctx, value_t value);
value_t value_unref_assigment(value_t self, struct _context_t *ctx,
                              value_t value);
bool value_is_interrupt(value_t value);
bool value_is_error(value_t value);

value_t value_to_string(value_t self, struct _context_t *ctx);
value_t value_get_index(value_t self, struct _context_t *ctx, size_t idx);
value_t value_set_index(value_t self, struct _context_t *ctx, size_t idx,
                        value_t item);
value_t value_get_field(value_t self, struct _context_t *ctx, const char *name);
value_t value_set_field(value_t self, struct _context_t *ctx, const char *name,
                        value_t value);
value_t value_get_length(value_t self, struct _context_t *ctx);
value_t value_call(value_t self, struct _context_t *ctx, size_t argc,
                   value_t argv[]);
value_t value_convert(value_t self, struct _context_t *ctx, type_t type);
value_t value_safe_convert(value_t self, struct _context_t *ctx, type_t type);
value_t value_add(value_t self, struct _context_t *ctx, value_t another);
value_t value_sub(value_t self, struct _context_t *ctx, value_t another);
value_t value_mul(value_t self, struct _context_t *ctx, value_t another);
value_t value_div(value_t self, struct _context_t *ctx, value_t another);
value_t value_mod(value_t self, struct _context_t *ctx, value_t another);
value_t value_and(value_t self, struct _context_t *ctx, value_t another);
value_t value_or(value_t self, struct _context_t *ctx, value_t another);
value_t value_xor(value_t self, struct _context_t *ctx, value_t another);
value_t value_shl(value_t self, struct _context_t *ctx, value_t another);
value_t value_shr(value_t self, struct _context_t *ctx, value_t another);
value_t value_eq(value_t self, struct _context_t *ctx, value_t another);
value_t value_ne(value_t self, struct _context_t *ctx, value_t another);
value_t value_lt(value_t self, struct _context_t *ctx, value_t another);
value_t value_gt(value_t self, struct _context_t *ctx, value_t another);
value_t value_le(value_t self, struct _context_t *ctx, value_t another);
value_t value_ge(value_t self, struct _context_t *ctx, value_t another);
value_t value_plus(value_t self, struct _context_t *ctx);
value_t value_neg(value_t self, struct _context_t *ctx);
value_t value_bitwise_not(value_t self, struct _context_t *ctx);
value_t value_logical_not(value_t self, struct _context_t *ctx);
value_t value_logical_and(value_t self, struct _context_t *ctx,
                          value_t another);
value_t value_logical_or(value_t self, struct _context_t *ctx, value_t another);
value_t value_unref(value_t self, struct _context_t *ctx);
value_t value_ref(value_t self, struct _context_t *ctx);
value_t value_try(value_t self, struct _context_t *ctx);
#ifdef __cplusplus
}
#endif
#endif