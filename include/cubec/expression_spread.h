#ifndef _H_CUBEC_CUBEC_EXPRESSION_SPREAD_
#define _H_CUBEC_CUBEC_EXPRESSION_SPREAD_
#include "engine/context.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/expression.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_expression_spread_t;
struct _cubec_expression_spread_t {
  struct _cubec_expression_t super;
  node_t value; /**< The expression being spread */
};
typedef struct _cubec_expression_spread_t *cubec_expression_spread_t;

extern type_t g_cubec_expression_spread_type;

struct _cubec_expression_spread_init_t {
  location_t location;
  node_t parent;
  node_t value;
};
typedef struct _cubec_expression_spread_init_t cubec_expression_spread_init_t;

/**
 * @brief Try to parse a spread expression: \c ...value
 *
 * This is NOT part of the normal expression parsing tree.
 * It is explicitly called in contexts that support spread syntax
 * (function call arguments, function parameter lists, struct initializers, etc.).
 *
 * @return A new cubec_expression_spread_t node, or NULL if the current
 *         token is not three consecutive '.' symbols.
 */
node_t read_expression_spread(context_t ctx, vec_t tokens,
                              size_t *position, const char *filename);

node_t cubec_ast_create_spread(context_t ctx, location_t loc, node_t value);

#ifdef __cplusplus
}
#endif
#endif
