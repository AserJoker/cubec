#ifndef _H_CUBEC_CUBEC_EXPRESSION_TYPEOF_
#define _H_CUBEC_CUBEC_EXPRESSION_TYPEOF_
#include "core/allocator.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/expression.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_expression_typeof_t;
struct _cubec_expression_typeof_t {
  struct _cubec_expression_t super;
  node_t expression; /**< The expression whose type is computed at compile time */
};
typedef struct _cubec_expression_typeof_t *cubec_expression_typeof_t;

extern type_t g_cubec_expression_typeof_type;

struct _cubec_expression_typeof_init_t {
  location_t location;
  node_t parent;
  node_t expression;
};
typedef struct _cubec_expression_typeof_init_t
    cubec_expression_typeof_init_t;

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
node_t read_expression_typeof(allocator_t allocator, vec_t tokens,
                               size_t *position, const char *filename);

#ifdef __cplusplus
}
#endif
#endif
