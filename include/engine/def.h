#ifndef _H_CUBEC_ENGINE_DEF_
#define _H_CUBEC_ENGINE_DEF_

#include "core/allocator.h"
#include "core/node.h"
#ifdef __cplusplus
extern "C" {
#endif

/* --------------------------------------------------------------------------
 *  Definition kinds
 * -------------------------------------------------------------------------- */

enum def_kind {
  DEF_FUNC,
  DEF_STRUCT,
  DEF_UNION,
  DEF_ENUM,
  DEF_INTERFACE,
  DEF_TYPE_ALIAS,
  DEF_VAR,
  DEF_NAMESPACE,
  DEF_CUNION,
};

/* --------------------------------------------------------------------------
 *  Base definition type
 * -------------------------------------------------------------------------- */

struct _def_t {
  allocator_t allocator;
  enum def_kind kind;
  node_t node; /* borrowing: original AST declaration node */
};

typedef struct _def_t *def_t;

/* --------------------------------------------------------------------------
 *  Per-declaration definition types
 *  Each embeds _def_t as the first field (super) for def_t coercion.
 *  Additional fields will be filled in during subsequent design iterations.
 * -------------------------------------------------------------------------- */

struct _module_t;
typedef struct _module_t *module_t;

/* --------------------------------------------------------------------------
 *  Generic parameter definition (placeholder for now)
 * -------------------------------------------------------------------------- */

typedef struct _param_def_t *param_def_t;
struct _param_def_t {
  allocator_t allocator;
  /* TODO: next stage fills: constraints, value_type, is_rest */
};

typedef struct _func_def_t *func_def_t;
struct _func_def_t {
  struct _def_t super;
  /* TODO: generic_params, arguments, return_type, body, flags */
};

typedef struct _struct_def_t *struct_def_t;
struct _struct_def_t {
  struct _def_t super;
  strmap_t params;     /* owned: param name → param_def_t */
  vec_t implements;    /* NULL for now, filled next stage */
  vec_t members;       /* NULL for now, filled next stage */
};

typedef struct _union_def_t *union_def_t;
struct _union_def_t {
  struct _def_t super;
  strmap_t params;     /* owned: param name → param_def_t */
  vec_t implements;    /* NULL for now, filled next stage */
  vec_t members;       /* NULL for now, filled next stage */
};

typedef struct _enum_def_t *enum_def_t;
struct _enum_def_t {
  struct _def_t super;
  vec_t items;         /* NULL for now, filled next stage */
};

typedef struct _interface_def_t *interface_def_t;
struct _interface_def_t {
  struct _def_t super;
  strmap_t params;     /* owned: param name → param_def_t */
  vec_t members;       /* NULL for now, filled next stage */
};

typedef struct _type_alias_def_t *type_alias_def_t;
struct _type_alias_def_t {
  struct _def_t super;
  strmap_t params;     /* owned: param name → param_def_t (empty for now) */
  node_t type_value;   /* NULL in this stage, filled later */
  bool is_builtin;
};

typedef struct _var_def_t *var_def_t;
struct _var_def_t {
  struct _def_t super;
  /* TODO: type annotation, initializer, is_const, flags */
};

typedef struct _namespace_def_t *namespace_def_t;
struct _namespace_def_t {
  struct _def_t super;
  struct _module_t *module; /* borrowing: the imported module */
};

typedef struct _cunion_def_t *cunion_def_t;
struct _cunion_def_t {
  struct _def_t super;
  vec_t members;       /* NULL for now, filled next stage */
};

#ifdef __cplusplus
}
#endif

#endif /* _H_CUBEC_ENGINE_DEF_ */
