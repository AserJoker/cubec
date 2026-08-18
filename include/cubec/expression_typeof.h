#ifndef _H_CUBEC_CUBEC_EXPRESSION_TYPEOF_
#define _H_CUBEC_CUBEC_EXPRESSION_TYPEOF_
#include "core/location.h"
#include "core/node.h"
#include "core/class.h"
#include "core/vec.h"
#include "core/emit_context.h"
#include "cubec/expression.h"
#include "engine/vm.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_expression_typeof_t;
struct _cubec_expression_typeof_t {
  struct _cubec_expression_t super;
  node_t
      expression; /**< The expression whose type is computed at compile time */
};
typedef struct _cubec_expression_typeof_t *cubec_expression_typeof_t;

extern class_t g_cubec_expression_typeof_class;

struct _cubec_expression_typeof_init_t {
  location_t location;
  node_t parent;
  node_t expression;
};
typedef struct _cubec_expression_typeof_init_t cubec_expression_typeof_init_t;

/**
 * @brief Try to parse a typeof expression: \c typeof(<expression>)
 *
 * typeof is a compile-time construct that computes the type of an expression
 * without evaluating it. It is a top-level type expression (like ternary
 * type conditionals) and cannot be used as a base type in pointer/slice/array
 * declarations. Postfix namespace access (e.g. typeof(File)::open) and
 * generic instantiation (e.g. typeof(Vec)[i32]) are still supported.
 *
 * @return A new cubec_expression_typeof_t node, or NULL if the current token
 *         is not the \c typeof keyword.
 */
node_t read_expression_typeof(vm_t vm, vec_t tokens, size_t *position,
                              const char *filename);

node_t create_expression_typeof(vm_t vm, location_t loc, node_t expr);


void emit_expression_typeof(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
