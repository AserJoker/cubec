#ifndef _H_CUBEC_CUBEC_EXPRESSION_TYPE_QUALIFIER_
#define _H_CUBEC_CUBEC_EXPRESSION_TYPE_QUALIFIER_
#include "core/allocator.h"
#include "engine/context.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/expression.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_expression_type_qualifier_t;
struct _cubec_expression_type_qualifier_t {
  struct _cubec_expression_t super;
  node_t type;        /**< The underlying type expression */
  bool is_const;      /**< true = const qualifier */
  bool is_volatile;   /**< true = volatile qualifier */
};
typedef struct _cubec_expression_type_qualifier_t *cubec_expression_type_qualifier_t;

extern type_t g_cubec_expression_type_qualifier_type;

struct _cubec_expression_type_qualifier_init_t {
  location_t location;
  node_t parent;
  node_t type;
  bool is_const;
  bool is_volatile;
};
typedef struct _cubec_expression_type_qualifier_init_t cubec_expression_type_qualifier_init_t;

/**
 * @brief Try to parse a type qualifier expression: const <type> or volatile <type>
 * The underlying type is parsed by read_type_expression_primary,
 * which does NOT include ternary — use type_group for that:
 *   const (a ? b : c)
 * @return A new cubec_expression_type_qualifier_t node, or NULL if the current
 *         token is not the keyword const or volatile.
 */
node_t read_expression_type_qualifier(context_t ctx, vec_t tokens,
                                      size_t *position, const char *filename);

#ifdef __cplusplus
}
#endif
#endif
