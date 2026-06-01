#ifndef _H_ENGINE_TYPE_
#define _H_ENGINE_TYPE_
#include "core/allocator.h"
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum _type_kind_t {
  TYPE_KIND_INTERRUPT,
  TYPE_KIND_ERROR,
  TYPE_KIND_NIL,
  TYPE_KIND_INFER,
  TYPE_KIND_VOID,
  TYPE_KIND_BOOL,
  TYPE_KIND_STR,
  TYPE_KIND_TYPE,
  TYPE_KIND_I8,
  TYPE_KIND_I16,
  TYPE_KIND_I32,
  TYPE_KIND_I64,
  TYPE_KIND_U8,
  TYPE_KIND_U16,
  TYPE_KIND_U32,
  TYPE_KIND_U64,
  TYPE_KIND_F16,
  TYPE_KIND_F32,
  TYPE_KIND_F64,
  TYPE_KIND_STRUCT,
  TYPE_KIND_ENUM,
  TYPE_KIND_UNION,
  TYPE_KIND_PTR,
  TYPE_KIND_PARRAY,
  TYPE_KIND_SLICE,
  TYPE_KIND_ARRAY,
  TYPE_KIND_FUNCTION,
  TYPE_KIND_TEMPLATE,
} type_kind_t;

struct _value_t;
struct _context_t;

typedef struct _type_t *type_t;
typedef struct _type_operator_t *type_operator_t;
struct _type_operator_t {
  bool (*type_equal)(type_t self, type_t another);
  struct _value_t *(*safe_convert)(struct _value_t *, struct _context_t *,
                                   type_t);
  struct _value_t *(*len)(struct _value_t *, struct _context_t *);
  struct _value_t *(*slice)(struct _value_t *, struct _context_t *,
                            struct _value_t *, struct _value_t *);
  struct _value_t *(*call)(struct _value_t *, struct _context_t *, size_t,
                           struct _value_t **);
  struct _value_t *(*get)(struct _value_t *, struct _context_t *,
                          struct _value_t *);
  struct _value_t *(*set)(struct _value_t *, struct _context_t *,
                          struct _value_t *, struct _value_t *);
  struct _value_t *(*get_field)(struct _value_t *, struct _context_t *,
                                const char *);
  struct _value_t *(*set_field)(struct _value_t *, struct _context_t *,
                                const char *, struct _value_t *);
  struct _value_t *(*iterator)(struct _value_t *, struct _context_t *);
  struct _value_t *(*opt_add)(struct _value_t *, struct _context_t *,
                              struct _value_t *);
  struct _value_t *(*opt_sub)(struct _value_t *, struct _context_t *,
                              struct _value_t *);
  struct _value_t *(*opt_mod)(struct _value_t *, struct _context_t *,
                              struct _value_t *);
  struct _value_t *(*opt_mul)(struct _value_t *, struct _context_t *,
                              struct _value_t *);
  struct _value_t *(*opt_div)(struct _value_t *, struct _context_t *,
                              struct _value_t *);
  struct _value_t *(*opt_shr)(struct _value_t *, struct _context_t *,
                              struct _value_t *);
  struct _value_t *(*opt_shl)(struct _value_t *, struct _context_t *,
                              struct _value_t *);
  struct _value_t *(*opt_and)(struct _value_t *, struct _context_t *,
                              struct _value_t *);
  struct _value_t *(*opt_or)(struct _value_t *, struct _context_t *,
                             struct _value_t *);
  struct _value_t *(*opt_xor)(struct _value_t *, struct _context_t *,
                              struct _value_t *);
  struct _value_t *(*opt_eq)(struct _value_t *, struct _context_t *,
                             struct _value_t *);
  struct _value_t *(*opt_ne)(struct _value_t *, struct _context_t *,
                             struct _value_t *);
  struct _value_t *(*opt_gt)(struct _value_t *, struct _context_t *,
                             struct _value_t *);
  struct _value_t *(*opt_ge)(struct _value_t *, struct _context_t *,
                             struct _value_t *);
  struct _value_t *(*opt_lt)(struct _value_t *, struct _context_t *,
                             struct _value_t *);
  struct _value_t *(*opt_le)(struct _value_t *, struct _context_t *,
                             struct _value_t *);
  struct _value_t *(*opt_plu)(struct _value_t *, struct _context_t *);
  struct _value_t *(*opt_neg)(struct _value_t *, struct _context_t *);
  struct _value_t *(*opt_lnot)(struct _value_t *, struct _context_t *);
  struct _value_t *(*opt_not)(struct _value_t *, struct _context_t *);
};

struct _type_t {
  type_kind_t kind;
  char *name;
  char *id;
  size_t align;
  size_t size;
  void *meta;
  struct _type_operator_t opt;
  bool comptime;
};

typedef struct _ctype_t *ctype_t;
struct _ctype_t {
  type_t type;
  bool mut;
};

ctype_t create_ctype(allocator_t allocator, type_t type, bool mut);

type_t create_type(allocator_t allocator, type_kind_t kind, const char *name,
                   const char *id, size_t size, size_t align,
                   type_operator_t opt, void *meta, bool comptime);

bool type_is_equal(type_t self, type_t another);

void init_type_type(struct _context_t *ctx);

struct _value_t *create_type_value(struct _context_t *ctx, type_t type,
                                   bool mut, const char *name);
#ifdef __cplusplus
}
#endif
#endif