#ifndef _H_CUBEC_CUBEC_EXPRESSION_TYPE_VOLATILE_
#define _H_CUBEC_CUBEC_EXPRESSION_TYPE_VOLATILE_
#include "core/allocator.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/expression.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_expression_type_volatile_t;
struct _cubec_expression_type_volatile_t {
  struct _cubec_expression_t super;
  node_t type; /**< The underlying type expression */
};
typedef struct _cubec_expression_type_volatile_t *cubec_expression_type_volatile_t;

extern type_t g_cubec_expression_type_volatile_type;

struct _cubec_expression_type_volatile_init_t {
  location_t location;
  node_t parent;
  node_t type;
};
typedef struct _cubec_expression_type_volatile_init_t cubec_expression_type_volatile_init_t;

/**
 * @brief Try to parse a volatile type expression: \c volatile <type>
 * The underlying type is parsed by read_type_expression_primary,
 * which does NOT include ternary — use type_group for that:
 *   volatile (a ? b : c)
 * @return A new cubec_expression_type_volatile_t node, or NULL if the current
 *         token is not the keyword \c volatile.
 */
node_t read_expression_type_volatile(allocator_t allocator, vec_t tokens,
                                     size_t *position, const char *filename);

#ifdef __cplusplus
}
#endif
#endif
