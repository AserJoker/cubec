#ifndef _H_CUBEC_ENGINE_TYPE_
#define _H_CUBEC_ENGINE_TYPE_
#include "core/allocator.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum _type_kind_t {
  VALUE_TYPE_ERROR,
  VALUE_TYPE_ANY,
  VALUE_TYPE_INTERRUPT,
  VALUE_TYPE_BUILTIN,
  VALUE_TYPE_VOID,
  VALUE_TYPE_TYPE,
  VALUE_TYPE_BOOL,
  VALUE_TYPE_INT8,
  VALUE_TYPE_INT16,
  VALUE_TYPE_INT32,
  VALUE_TYPE_INT64,
  VALUE_TYPE_UINT8,
  VALUE_TYPE_UINT16,
  VALUE_TYPE_UINT32,
  VALUE_TYPE_UINT64,
  VALUE_TYPE_FLOAT32,
  VALUE_TYPE_FLOAT64,
  VALUE_TYPE_STR,
  VALUE_TYPE_PTR,
  VALUE_TYPE_PARRAY,
  VALUE_TYPE_OPAQUE,
  VALUE_TYPE_ARRAY,
  VALUE_TYPE_STRUCT,
  VALUE_TYPE_UNION,
  VALUE_TYPE_FUNCTION,
} type_kind_t;

struct _value_t;

struct _context_t;
typedef struct _type_t *type_t;
typedef bool (*type_is_equal_fn_t)(type_t self, type_t another);

typedef char *(*type_to_string_fn_t)(type_t self, allocator_t allocator);

typedef struct _value_t *(*get_field_fn_t)(struct _value_t *self,
                                           struct _context_t *ctx,
                                           const char *name);

typedef struct _value_t *(*set_field_fn_t)(struct _value_t *self,
                                           struct _context_t *ctx,
                                           const char *name,
                                           struct _value_t *value);

typedef struct _value_t *(*get_index_fn_t)(struct _value_t *self,
                                           struct _context_t *ctx, size_t idx);

typedef struct _value_t *(*set_index_fn_t)(struct _value_t *self,
                                           struct _context_t *ctx, size_t idx,
                                           struct _value_t *value);

typedef struct _value_t *(*get_length_fn_t)(struct _value_t *self,
                                            struct _context_t *ctx);

typedef struct _value_t *(*to_string_fn_t)(struct _value_t *self,
                                           struct _context_t *ctx);

typedef struct _value_t *(*call_fn_t)(struct _value_t *self,
                                      struct _context_t *ctx, size_t argc,
                                      struct _value_t *argv[]);

typedef struct _value_t *(*binary_fn_t)(struct _value_t *self,
                                        struct _context_t *ctx,
                                        struct _value_t *rvalue);
typedef struct _value_t *(*single_fn_t)(struct _value_t *self,
                                        struct _context_t *ctx);

typedef struct _value_t *(*convert_fn_t)(struct _value_t *self,
                                         struct _context_t *ctx, type_t type);

typedef struct _type_operator_t {
  type_is_equal_fn_t is_type_equal;
  type_to_string_fn_t type_to_string;
  to_string_fn_t to_string;
  get_field_fn_t get_field;
  set_field_fn_t set_field;
  get_index_fn_t get_index;
  set_index_fn_t set_index;
  get_length_fn_t get_length;
  call_fn_t call;
  convert_fn_t convert;
  binary_fn_t add_opt;
  binary_fn_t sub_opt;
  binary_fn_t div_opt;
  binary_fn_t mul_opt;
  binary_fn_t mod_opt;
  binary_fn_t and_opt;
  binary_fn_t or_opt;
  binary_fn_t xor_opt;
  binary_fn_t shl_opt;
  binary_fn_t shr_opt;
  binary_fn_t eq_opt;
  binary_fn_t ne_opt;
  binary_fn_t lt_opt;
  binary_fn_t gt_opt;
  binary_fn_t le_opt;
  binary_fn_t ge_opt;
  binary_fn_t logical_and_opt;
  binary_fn_t logical_or_opt;
  single_fn_t plus_opt;
  single_fn_t neg_opt;
  single_fn_t logical_not_opt;
  single_fn_t bitwise_not_opt;
  single_fn_t ref;
  single_fn_t unref;
} *type_operator_t;
type_t create_type(allocator_t allocator, type_kind_t kind, size_t size,
                   size_t align, void *meta, type_operator_t opt);
type_kind_t type_get_kind(type_t self);
type_operator_t type_get_operator(type_t self);
size_t type_get_size(type_t self);
void type_set_size(type_t self, size_t size);
void *type_get_meta(type_t self);
size_t type_get_align(type_t self);
void type_set_align(type_t self, size_t align);
bool type_is_equal(type_t self, type_t another);
const char *type_kind_to_string(type_kind_t kind);
char *type_to_string(type_t self, allocator_t allocator);
struct _value_t *create_type_value(struct _context_t *ctx, type_t type,
                                   bool mutable, const char *name);
#ifdef __cplusplus
}
#endif
#endif