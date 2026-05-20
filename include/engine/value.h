#ifndef _H_ENGINE_VALUE_
#define _H_ENGINE_VALUE_
#include "core/allocator.h"
#include "engine/type.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _value_t *value_t;
struct _value_t {
  type_t type;
  void *data;
  bool mut;
  bool comptime;
};
struct _context_t;
value_t create_value(allocator_t allocator, type_t type, bool mut);
value_t create_comptime_value(allocator_t allocator, type_t type,
                              const void *data, bool mut);
value_t create_weak_value(allocator_t allocator, type_t type, void *data,
                          bool mut);
value_t value_clone(value_t self, allocator_t allcoator);
value_t value_ref(value_t self, allocator_t allcoator);
value_t value_safe_convert(value_t self, struct _context_t *ctx, type_t type);
value_t value_addr(value_t self, struct _context_t *ctx);
value_t value_deref(value_t self, struct _context_t *ctx);
value_t value_len(value_t self, struct _context_t *ctx);
value_t value_slice(value_t self, struct _context_t *ctx, value_t start,
                    value_t end);
value_t value_call(value_t self, struct _context_t *ctx, size_t argc,
                   value_t *argv);
value_t value_get(value_t self, struct _context_t *ctx, value_t field);
value_t value_set(value_t self, struct _context_t *ctx, value_t feild,
                  value_t value);
value_t value_get_field(value_t self, struct _context_t *ctx,
                        const char *field);
value_t value_set_field(value_t self, struct _context_t *ctx, const char *field,
                        value_t value);
value_t value_iterator(value_t self, struct _context_t *ctx);
value_t value_opt_add(value_t self, struct _context_t *ctx, value_t another);
value_t value_opt_sub(value_t self, struct _context_t *ctx, value_t another);
value_t value_opt_mod(value_t self, struct _context_t *ctx, value_t another);
value_t value_opt_mul(value_t self, struct _context_t *ctx, value_t another);
value_t value_opt_div(value_t self, struct _context_t *ctx, value_t another);
value_t value_opt_shr(value_t self, struct _context_t *ctx, value_t another);
value_t value_opt_shl(value_t self, struct _context_t *ctx, value_t another);
value_t value_opt_and(value_t self, struct _context_t *ctx, value_t another);
value_t value_opt_or(value_t self, struct _context_t *ctx, value_t another);
value_t value_opt_xor(value_t self, struct _context_t *ctx, value_t another);
value_t value_opt_eq(value_t self, struct _context_t *ctx, value_t another);
value_t value_opt_ne(value_t self, struct _context_t *ctx, value_t another);
value_t value_opt_gt(value_t self, struct _context_t *ctx, value_t another);
value_t value_opt_ge(value_t self, struct _context_t *ctx, value_t another);
value_t value_opt_lt(value_t self, struct _context_t *ctx, value_t another);
value_t value_opt_le(value_t self, struct _context_t *ctx, value_t another);
value_t value_opt_land(value_t self, struct _context_t *ctx, value_t another);
value_t value_opt_lor(value_t self, struct _context_t *ctx, value_t another);
value_t value_opt_plu(value_t self, struct _context_t *ctx);
value_t value_opt_neg(value_t self, struct _context_t *ctx);
value_t value_opt_lnot(value_t self, struct _context_t *ctx);
value_t value_opt_not(value_t self, struct _context_t *ctx);
value_t value_assigment(value_t self, struct _context_t *ctx, value_t value);
#ifdef __cplusplus
}
#endif
#endif