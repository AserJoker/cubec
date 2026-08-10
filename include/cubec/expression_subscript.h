#ifndef _H_CUBEC_CUBEC_EXPRESSION_SUBSCRIPT_
#define _H_CUBEC_CUBEC_EXPRESSION_SUBSCRIPT_
#include "engine/context.h"
#include "core/location.h"
#include "core/node.h"
#include "core/class.h"
#include "core/emit_context.h"
#include "cubec/expression.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_expression_subscript_t;
struct _cubec_expression_subscript_t {
  struct _cubec_expression_t super;
  node_t host;    /**< The expression being subscripted (e.g., a tuple) */
  node_t index;   /**< Index expression (must be comptime-evaluable) */
};
typedef struct _cubec_expression_subscript_t *cubec_expression_subscript_t;

extern class_t g_cubec_expression_subscript_class;

struct _cubec_expression_subscript_init_t {
  location_t location;
  node_t parent;
  node_t host;
  node_t index;
};
typedef struct _cubec_expression_subscript_init_t cubec_expression_subscript_init_t;

/**
 * @brief Try to parse a subscript expression: host[index]
 *
 * Called from read_value as a postfix operator after the host expression
 * has been parsed. Checks for '[' at the current position.
 *
 * A subscript is distinguished from a slice (host[start:length]) by the
 * absence of a ':' token, and from generic instantiation (callee[Type])
 * by the host not being a plain identifier.
 *
 * @param host The node already parsed as the expression being subscripted.
 *             Ownership is transferred to the new subscript node.
 * @return A new cubec_expression_subscript_t node, or NULL if the next token
 *         is not a valid subscript (position is NOT advanced on NULL return).
 */
node_t read_expression_subscript(context_t ctx, vec_t tokens,
                                 size_t *position, const char *filename,
                                 node_t host);

node_t create_expression_subscript(context_t ctx, location_t loc, node_t host,
                                   node_t index);


void emit_expression_subscript(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
