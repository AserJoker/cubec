#ifndef _H_CUBEC_CUBEC_EXPRESSION_COMMA_
#define _H_CUBEC_CUBEC_EXPRESSION_COMMA_
#include "core/emit_context.h"
#include "core/location.h"
#include "core/node.h"
#include "core/class.h"
#include "core/vec.h"
#include "cubec/expression.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_expression_comma_t;
struct _cubec_expression_comma_t {
  struct _cubec_expression_t super;
  node_t left;  /**< Left operand (expression, assignment, or another comma) */
  node_t right; /**< Right operand (must be a non-comma expression) */
};
typedef struct _cubec_expression_comma_t *cubec_expression_comma_t;

extern class_t g_cubec_expression_comma_class;

struct _cubec_expression_comma_init_t {
  location_t location;
  node_t parent;
  node_t left;  /**< Left operand */
  node_t right; /**< Right operand */
};
typedef struct _cubec_expression_comma_init_t cubec_expression_comma_init_t;

/**
 * @brief Try to parse a comma expression: \c left, right
 *        The left side can be an expression, assignment, or another comma
 * expression. The right side is a non-comma expression.
 *
 * @return A new cubec_expression_comma_t node, or NULL if the current
 *         token is not a comma operator.
 */
node_t read_expression_comma(context_t ctx, vec_t tokens, size_t *position,
                             const char *filename);

node_t create_expression_comma(context_t ctx, location_t loc, node_t left,
                               node_t right);


void emit_expression_comma(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
