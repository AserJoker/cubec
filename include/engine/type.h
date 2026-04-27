#ifndef _H_ENGINE_TYPE_
#define _H_ENGINE_TYPE_
#include "ast/node.h"
#include "core/allocator.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum _type_kind_t {
  TYPE_KIND_INTERRUPT,
  TYPE_KIND_ERROR,
  TYPE_KIND_VOID,
  TYPE_KIND_NULL,
  TYPE_KIND_TYPE,
  TYPE_KIND_INTEGER,
  TYPE_KIND_UNSIGNED,
  TYPE_KIND_FLOAT,
  TYPE_KIND_BOOL,
  TYPE_KIND_STR,
  TYPE_KIND_PTR,
  TYPE_KIND_PARRAY,
  TYPE_KIND_OPAQUE,
  TYPE_KIND_ARRAY,
  TYPE_KIND_STRUCT,
  TYPE_KIND_UNION,
  TYPE_KIND_FUNCTION,
} type_kind_t;

struct _value_t;
struct _context_t;
struct _type_t;
typedef struct _type_t *type_t;
typedef struct _value_t *(*single_fn_t)(struct _value_t *self,
                                        struct _context_t *ctx);
typedef struct _value_t *(*binary_fn_t)(struct _value_t *self,
                                        struct _context_t *ctx,
                                        struct _value_t *another);
typedef struct _value_t *(*convert_fn_t)(struct _value_t *self,
                                         struct _context_t *ctx,
                                         struct _type_t *type);
typedef struct _value_t *(*get_field_fn_t)(struct _value_t *self,
                                           struct _context_t *ctx,
                                           const char *name);
typedef struct _value_t *(*set_field_fn_t)(struct _value_t *self,
                                           struct _context_t *ctx,
                                           const char *name,
                                           struct _value_t *value);
typedef struct _value_t *(*get_fn_t)(struct _value_t *self,
                                     struct _context_t *ctx,
                                     struct _value_t *key);
typedef struct _value_t *(*set_fn_t)(struct _value_t *self,
                                     struct _context_t *ctx,
                                     struct _value_t *key,
                                     struct _value_t *value);
typedef struct _value_t *(*get_length_fn_t)(struct _value_t *self,
                                            struct _context_t *ctx);
typedef struct _value_t *(*call_fn_t)(struct _value_t *self,
                                      struct _context_t *ctx, size_t argc,
                                      struct _value_t *argv[]);

typedef char *(*write_ast_fn_t)(struct _value_t *self, allocator_t allocator);

typedef bool (*type_eq_fn_t)(type_t self, type_t another);

typedef struct _type_operator_t {
  type_eq_fn_t type_eq;
  single_fn_t addr_of;
  single_fn_t deref;
  single_fn_t plus;
  single_fn_t neg;
  single_fn_t logical_not;
  single_fn_t bitwise_not;

  binary_fn_t add;
  binary_fn_t sub;
  binary_fn_t mul;
  binary_fn_t div;
  binary_fn_t mod;
  binary_fn_t and_;
  binary_fn_t or_;
  binary_fn_t xor_;
  binary_fn_t shl;
  binary_fn_t shr;
  binary_fn_t eq;
  binary_fn_t ne;
  binary_fn_t gt;
  binary_fn_t ge;
  binary_fn_t lt;
  binary_fn_t le;
  binary_fn_t logical_and;
  binary_fn_t logical_or;
  binary_fn_t assigment;

  get_field_fn_t get_field;
  set_field_fn_t set_field;

  get_fn_t get;
  set_fn_t set;

  get_length_fn_t get_length;

  call_fn_t call;

  convert_fn_t convert;
  convert_fn_t safe_convert;
} type_operator_t;
type_t create_type(allocator_t allocator, type_kind_t kind, size_t size,
                   size_t align, const char *name, const char *id,
                   type_operator_t *opt, void *meta);
type_kind_t type_get_kind(type_t self);
size_t type_get_size(type_t self);
size_t type_get_align(type_t self);
void type_set_size(type_t self, size_t size);
void type_set_align(type_t self, size_t align);
const char *type_get_name(type_t self);
const char *type_get_id(type_t self);
const type_operator_t *type_get_operator(type_t self);
void *type_get_meta(type_t self);

void type_init(struct _context_t *ctx);

struct _value_t *create_type_value(struct _context_t *ctx, type_t type,
                                   bool mut, const char *name);
bool type_default_eq(type_t self, type_t another);
bool type_is_equal(type_t self, type_t another);
#ifdef __cplusplus
}
#endif
#endif