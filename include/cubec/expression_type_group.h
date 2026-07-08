#ifndef _H_CUBEC_CUBEC_EXPRESSION_TYPE_GROUP_
#define _H_CUBEC_CUBEC_EXPRESSION_TYPE_GROUP_
#include "core/allocator.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/expression.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_expression_type_group_t;
struct _cubec_expression_type_group_t {
  struct _cubec_expression_t super;
  node_t inner; /**< The type expression inside the parentheses */
};
typedef struct _cubec_expression_type_group_t *cubec_expression_type_group_t;

extern type_t g_cubec_expression_type_group_type;

struct _cubec_expression_type_group_init_t {
  location_t location;
  node_t parent;
  node_t inner;
};
typedef struct _cubec_expression_type_group_init_t cubec_expression_type_group_init_t;

/**
 * @brief Try to parse a grouped type expression: \c ( type_expression )
 * @return A new cubec_expression_type_group_t node, or NULL if the current
 *         token is not \c '('.
 */
node_t read_expression_type_group(allocator_t allocator, vec_t tokens,
                                  size_t *position, const char *filename);

#ifdef __cplusplus
}
#endif
#endif