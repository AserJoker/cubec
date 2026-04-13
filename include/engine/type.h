#ifndef _H_CUBEC_ENGINE_TYPE_
#define _H_CUBEC_ENGINE_TYPE_
#include "core/allocator.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum _cubec_type_kind_t {
  CUBEC_VALUE_TYPE_ERROR,
  CUBEC_VALUE_TYPE_ANY,
  CUBEC_VALUE_TYPE_BUILTIN,
  CUBEC_VALUE_TYPE_VOID,
  CUBEC_VALUE_TYPE_TYPE,
  CUBEC_VALUE_TYPE_BOOL,
  CUBEC_VALUE_TYPE_INT8,
  CUBEC_VALUE_TYPE_INT16,
  CUBEC_VALUE_TYPE_INT32,
  CUBEC_VALUE_TYPE_INT64,
  CUBEC_VALUE_TYPE_UINT8,
  CUBEC_VALUE_TYPE_UINT16,
  CUBEC_VALUE_TYPE_UINT32,
  CUBEC_VALUE_TYPE_UINT64,
  CUBEC_VALUE_TYPE_FLOAT32,
  CUBEC_VALUE_TYPE_FLOAT64,
  CUBEC_VALUE_TYPE_STR,
  CUBEC_VALUE_TYPE_PTR,
  CUBEC_VALUE_TYPE_PARRAY,
  CUBEC_VALUE_TYPE_OPAQUE,
  CUBEC_VALUE_TYPE_ARRAY,
  CUBEC_VALUE_TYPE_STRUCT,
  CUBEC_VALUE_TYPE_UNION,
  CUBEC_VALUE_TYPE_FUNCTION,
} cubec_type_kind_t;

struct _cubec_value_t;
struct _cubec_context_t;
typedef struct _cubec_type_t *cubec_type_t;
typedef bool (*cubec_type_is_equal_fn_t)(cubec_type_t self,
                                         cubec_type_t another);

typedef char *(*cubec_type_to_string_fn_t)(cubec_type_t self,
                                           cubec_allocator_t allocator);

typedef struct _cubec_value_t *(*cubec_get_field_fn_t)(
    struct _cubec_value_t *self, struct _cubec_context_t *ctx,
    const char *name);

typedef struct _cubec_value_t *(*cubec_set_field_fn_t)(
    struct _cubec_value_t *self, struct _cubec_context_t *ctx, const char *name,
    struct _cubec_value_t *value);

typedef struct _cubec_value_t *(*cubec_get_index_fn_t)(
    struct _cubec_value_t *self, struct _cubec_context_t *ctx, size_t idx);

typedef struct _cubec_value_t *(*cubec_set_index_fn_t)(
    struct _cubec_value_t *self, struct _cubec_context_t *ctx, size_t idx,
    struct _cubec_value_t *value);

typedef struct _cubec_value_t *(*cubec_get_length_fn_t)(
    struct _cubec_value_t *self, struct _cubec_context_t *ctx);

typedef struct _cubec_value_t *(*cubec_to_string_fn_t)(
    struct _cubec_value_t *self, struct _cubec_context_t *ctx);

typedef struct _cubec_value_t *(*cubec_call_fn_t)(
    struct _cubec_value_t *self, struct _cubec_context_t *ctx, size_t argc,
    struct _cubec_value_t *argv[]);

typedef struct _cubec_value_t *(*cubec_binary_fn_t)(
    struct _cubec_value_t *self, struct _cubec_context_t *ctx,
    struct _cubec_value_t *rvalue);
typedef struct _cubec_value_t *(*cubec_single_fn_t)(
    struct _cubec_value_t *self, struct _cubec_context_t *ctx);

typedef struct _cubec_value_t *(*cubec_convert_fn_t)(
    struct _cubec_value_t *self, struct _cubec_context_t *ctx,
    cubec_type_t type);

typedef struct _cubec_type_operator_t {
  cubec_type_is_equal_fn_t is_type_equal;
  cubec_type_to_string_fn_t type_to_string;
  cubec_to_string_fn_t to_string;
  cubec_get_field_fn_t get_field;
  cubec_set_field_fn_t set_field;
  cubec_get_index_fn_t get_index;
  cubec_set_index_fn_t set_index;
  cubec_get_length_fn_t get_length;
  cubec_call_fn_t call;
  cubec_convert_fn_t convert;
  cubec_binary_fn_t add_opt;
  cubec_binary_fn_t sub_opt;
  cubec_binary_fn_t div_opt;
  cubec_binary_fn_t mul_opt;
  cubec_binary_fn_t mod_opt;
  cubec_binary_fn_t and_opt;
  cubec_binary_fn_t or_opt;
  cubec_binary_fn_t xor_opt;
  cubec_binary_fn_t shl_opt;
  cubec_binary_fn_t shr_opt;
  cubec_binary_fn_t eq_opt;
  cubec_binary_fn_t ne_opt;
  cubec_binary_fn_t lt_opt;
  cubec_binary_fn_t gt_opt;
  cubec_binary_fn_t le_opt;
  cubec_binary_fn_t ge_opt;
  cubec_binary_fn_t logical_and_opt;
  cubec_binary_fn_t logical_or_opt;
  cubec_single_fn_t plus_opt;
  cubec_single_fn_t neg_opt;
  cubec_single_fn_t logical_not_opt;
  cubec_single_fn_t bitwise_not_opt;
  cubec_single_fn_t prefix_inc;
  cubec_single_fn_t prefix_dec;
  cubec_single_fn_t postfix_inc;
  cubec_single_fn_t postfix_dec;
  cubec_single_fn_t ref;
  cubec_single_fn_t unref;
} *cubec_type_operator_t;
cubec_type_t cubec_create_type(cubec_allocator_t allocator,
                               cubec_type_kind_t kind, size_t size,
                               size_t align, void *meta,
                               cubec_type_operator_t opt);
cubec_type_kind_t cubec_type_get_kind(cubec_type_t self);
cubec_type_operator_t cubec_type_get_operator(cubec_type_t self);
size_t cubec_type_get_size(cubec_type_t self);
void cubec_type_set_size(cubec_type_t self, size_t size);
void *cubec_type_get_meta(cubec_type_t self);
size_t cubec_type_get_align(cubec_type_t self);
void cubec_type_set_align(cubec_type_t self, size_t align);
bool cubec_type_is_equal(cubec_type_t self, cubec_type_t another);
const char *cubec_type_kind_to_string(cubec_type_kind_t kind);
char *cubec_type_to_string(cubec_type_t self, cubec_allocator_t allocator);
struct _cubec_value_t *cubec_create_type_value(struct _cubec_context_t *ctx,
                                               cubec_type_t type, bool mutable,
                                               const char *name);
#ifdef __cplusplus
}
#endif
#endif