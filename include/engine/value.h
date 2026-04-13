#ifndef _H_CUBEC_ENGINE_VALUE_
#define _H_CUBEC_ENGINE_VALUE_
#include "core/allocator.h"
#include "engine/type.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_value_t *cubec_value_t;
struct _cubec_context_t;
cubec_value_t cubec_create_value(cubec_allocator_t allocator, cubec_type_t type,
                                 bool mutable, const void *data);
cubec_type_t cubec_value_get_type(cubec_value_t value);
bool cubec_value_type_is(cubec_value_t value, cubec_type_kind_t kind);
bool cubec_value_is_mutable(cubec_value_t value);
void cubec_value_set_mutable(cubec_value_t value, bool mutable);
void *cubec_value_get_data(cubec_value_t value);
cubec_value_t cubec_value_clone(cubec_allocator_t allocator,
                                cubec_value_t value);
cubec_value_t cubec_value_assigment(cubec_value_t self,
                                    struct _cubec_context_t *ctx,
                                    cubec_value_t value);
cubec_value_t cubec_value_unref_assigment(cubec_value_t self,
                                    struct _cubec_context_t *ctx,
                                    cubec_value_t value);
bool cubec_value_is_error(cubec_value_t value);

cubec_value_t cubec_value_to_string(cubec_value_t self,
                                    struct _cubec_context_t *ctx);
cubec_value_t cubec_value_get_index(cubec_value_t self,
                                    struct _cubec_context_t *ctx, size_t idx);
cubec_value_t cubec_value_set_index(cubec_value_t self,
                                    struct _cubec_context_t *ctx, size_t idx,
                                    cubec_value_t item);
cubec_value_t cubec_value_get_field(cubec_value_t self,
                                    struct _cubec_context_t *ctx,
                                    const char *name);
cubec_value_t cubec_value_set_field(cubec_value_t self,
                                    struct _cubec_context_t *ctx,
                                    const char *name, cubec_value_t value);
cubec_value_t cubec_value_get_length(cubec_value_t self,
                                     struct _cubec_context_t *ctx);
cubec_value_t cubec_value_call(cubec_value_t self, struct _cubec_context_t *ctx,
                               size_t argc, cubec_value_t argv[]);
cubec_value_t cubec_value_convert(cubec_value_t self,
                                  struct _cubec_context_t *ctx,
                                  cubec_type_t type);
cubec_value_t cubec_value_safe_convert(cubec_value_t self,
                                       struct _cubec_context_t *ctx,
                                       cubec_type_t type);
cubec_value_t cubec_value_add(cubec_value_t self, struct _cubec_context_t *ctx,
                              cubec_value_t another);
cubec_value_t cubec_value_sub(cubec_value_t self, struct _cubec_context_t *ctx,
                              cubec_value_t another);
cubec_value_t cubec_value_mul(cubec_value_t self, struct _cubec_context_t *ctx,
                              cubec_value_t another);
cubec_value_t cubec_value_div(cubec_value_t self, struct _cubec_context_t *ctx,
                              cubec_value_t another);
cubec_value_t cubec_value_mod(cubec_value_t self, struct _cubec_context_t *ctx,
                              cubec_value_t another);
cubec_value_t cubec_value_and(cubec_value_t self, struct _cubec_context_t *ctx,
                              cubec_value_t another);
cubec_value_t cubec_value_or(cubec_value_t self, struct _cubec_context_t *ctx,
                             cubec_value_t another);
cubec_value_t cubec_value_xor(cubec_value_t self, struct _cubec_context_t *ctx,
                              cubec_value_t another);
cubec_value_t cubec_value_shl(cubec_value_t self, struct _cubec_context_t *ctx,
                              cubec_value_t another);
cubec_value_t cubec_value_shr(cubec_value_t self, struct _cubec_context_t *ctx,
                              cubec_value_t another);
cubec_value_t cubec_value_eq(cubec_value_t self, struct _cubec_context_t *ctx,
                             cubec_value_t another);
cubec_value_t cubec_value_ne(cubec_value_t self, struct _cubec_context_t *ctx,
                             cubec_value_t another);
cubec_value_t cubec_value_lt(cubec_value_t self, struct _cubec_context_t *ctx,
                             cubec_value_t another);
cubec_value_t cubec_value_gt(cubec_value_t self, struct _cubec_context_t *ctx,
                             cubec_value_t another);
cubec_value_t cubec_value_le(cubec_value_t self, struct _cubec_context_t *ctx,
                             cubec_value_t another);
cubec_value_t cubec_value_ge(cubec_value_t self, struct _cubec_context_t *ctx,
                             cubec_value_t another);
cubec_value_t cubec_value_plus(cubec_value_t self,
                               struct _cubec_context_t *ctx);
cubec_value_t cubec_value_neg(cubec_value_t self, struct _cubec_context_t *ctx);
cubec_value_t cubec_value_bitwise_not(cubec_value_t self,
                                      struct _cubec_context_t *ctx);
cubec_value_t cubec_value_logical_not(cubec_value_t self,
                                      struct _cubec_context_t *ctx);
cubec_value_t cubec_value_logical_and(cubec_value_t self,
                                      struct _cubec_context_t *ctx,
                                      cubec_value_t another);
cubec_value_t cubec_value_logical_or(cubec_value_t self,
                                     struct _cubec_context_t *ctx,
                                     cubec_value_t another);
cubec_value_t cubec_value_unref(cubec_value_t self,
                                      struct _cubec_context_t *ctx);
cubec_value_t cubec_value_ref(cubec_value_t self,
                                      struct _cubec_context_t *ctx);
#ifdef __cplusplus
}
#endif
#endif