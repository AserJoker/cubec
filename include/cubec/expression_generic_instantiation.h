#ifndef _H_CUBEC_CUBEC_EXPRESSION_GENERIC_INSTANTIATION_
#define _H_CUBEC_CUBEC_EXPRESSION_GENERIC_INSTANTIATION_
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "core/writer.h"
#include "cubec/expression.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_expression_generic_instantiation_t;
struct _cubec_expression_generic_instantiation_t {
  struct _cubec_expression_t super;
  node_t callee;   /**< The expression being instantiated */
  vec_t arguments; /**< vec_t of generic argument expression nodes (may
                         contain spread expressions) */
};
typedef struct _cubec_expression_generic_instantiation_t
    *cubec_expression_generic_instantiation_t;

extern type_t g_cubec_expression_generic_instantiation_type;

struct _cubec_expression_generic_instantiation_init_t {
  location_t location;
  node_t parent;
  node_t callee;
  vec_t arguments; /**< Already-parsed arguments vec (transferred to node) */
};
typedef struct _cubec_expression_generic_instantiation_init_t
    cubec_expression_generic_instantiation_init_t;

/**
 * @brief Try to parse a generic-instantiation expression: \c callee[arg1, arg2,
 * ...]
 *
 * Called from \c read_value as a postfix operator after the callee has been
 * parsed. Checks for \c '[' at the current position, then parses a
 * comma-separated argument list until \c ']'.
 *
 * Each argument first tries \c read_expression_spread (to support \c ...expr
 * in generic arguments), then falls back to \c read_expression.
 *
 * @param callee  The node already parsed as the expression being instantiated.
 *                Ownership is transferred to the new node.
 * @return A new cubec_expression_generic_instantiation_t node, or NULL if the
 *         next token is not \c '[' (position is NOT advanced on NULL return).
 */
node_t read_expression_generic_instantiation(context_t ctx, vec_t tokens,
                                             size_t *position,
                                             const char *filename,
                                             node_t callee);

node_t create_expression_generic_instantiation(context_t ctx, location_t loc,
                                               node_t callee, vec_t args);

void write_expression_generic_instantiation(writer_t writer, node_t node);

#ifdef __cplusplus
}
#endif
#endif
