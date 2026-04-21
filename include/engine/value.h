#ifndef _H_ENGINE_VALUE_
#define _H_ENGINE_VALUE_
#include "core/allocator.h"
#include "type.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _value_t *value_t;
struct _context_t;
value_t create_value(allocator_t allocator, type_t type, bool mut,
                     const void *data, bool comptime);
value_t create_weak_value(allocator_t allocator, type_t type, bool mut,
                          void *data);
bool value_is_mutable(value_t value);
bool value_is_comptime(value_t value);
const void *value_get_data(value_t value);
type_t value_get_type(value_t value);
value_t value_clone(value_t self, allocator_t allocator);
value_t value_convert(value_t self, struct _context_t *ctx, type_t type);
value_t value_safe_convert(value_t self, struct _context_t *ctx, type_t type);
value_t value_addr_of(value_t self, struct _context_t *ctx);
value_t value_ref(value_t self, struct _context_t *ctx);
value_t value_deref(value_t self, struct _context_t *ctx);
value_t value_plus(value_t self, struct _context_t *ctx);
value_t value_neg(value_t self, struct _context_t *ctx);
value_t value_logical_not(value_t self, struct _context_t *ctx);
value_t value_bitwise_not(value_t self, struct _context_t *ctx);

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
value_t value_gt(value_t self, struct _context_t *ctx, value_t another);
value_t value_ge(value_t self, struct _context_t *ctx, value_t another);
value_t value_lt(value_t self, struct _context_t *ctx, value_t another);
value_t value_le(value_t self, struct _context_t *ctx, value_t another);
value_t value_logical_and(value_t self, struct _context_t *ctx,
                          value_t another);
value_t value_logical_or(value_t self, struct _context_t *ctx, value_t another);

value_t value_get_field(value_t self, struct _context_t *ctx, const char *name);
value_t value_set_field(value_t self, struct _context_t *ctx, const char *name,
                        value_t value);

value_t value_get_index(value_t self, struct _context_t *ctx, size_t idx);
value_t value_set_index(value_t self, struct _context_t *ctx, size_t idx,
                        value_t value);

value_t value_get_length(value_t self, struct _context_t *ctx);

value_t value_call(value_t self, struct _context_t *ctx, size_t argc,
                   value_t argv[]);
value_t value_assigment(value_t self, struct _context_t *ctx, value_t value);
value_t value_default_assigment(value_t self, struct _context_t *ctx,
                                value_t value);
#ifdef __cplusplus
}
#endif
#endif