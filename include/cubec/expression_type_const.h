#ifndef _H_CUBEC_CUBEC_EXPRESSION_TYPE_CONST_
#define _H_CUBEC_CUBEC_EXPRESSION_TYPE_CONST_
#include "core/allocator.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/expression.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_expression_type_const_t;
struct _cubec_expression_type_const_t {
  struct _cubec_expression_t super;
  node_t type; /**< The underlying type expression */
};
typedef struct _cubec_expression_type_const_t *cubec_expression_type_const_t;

extern type_t g_cubec_expression_type_const_type;

struct _cubec_expression_type_const_init_t {
  location_t location;
  node_t parent;
  node_t type;
};
typedef struct _cubec_expression_type_const_init_t cubec_expression_type_const_init_t;

/**
 * @brief Try to parse a const type expression: \c const <type>
 * The underlying type is parsed by read_type_expression_primary,
 * which does NOT include ternary — use type_group for that:
 *   const (a ? b : c)
 * @return A new cubec_expression_type_const_t node, or NULL if the current
 *         token is not the keyword \c const.
 */
node_t read_expression_type_const(allocator_t allocator, vec_t tokens,
                                  size_t *position, const char *filename);

#ifdef __cplusplus
}
#endif
#endif
