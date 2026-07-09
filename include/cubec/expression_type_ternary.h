#ifndef _H_CUBEC_CUBEC_EXPRESSION_TYPE_TERNARY_
#define _H_CUBEC_CUBEC_EXPRESSION_TYPE_TERNARY_
#include "core/allocator.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "cubec/expression.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_expression_type_ternary_t;
struct _cubec_expression_type_ternary_t {
  struct _cubec_expression_t super;
  node_t condition;  /**< Condition/value expression */
  node_t consequent; /**< Type expression when condition is true */
  node_t alternate;  /**< Type expression when condition is false */
};
typedef struct _cubec_expression_type_ternary_t *cubec_expression_type_ternary_t;

extern type_t g_cubec_expression_type_ternary_type;

struct _cubec_expression_type_ternary_init_t {
  location_t location;
  node_t parent;
  node_t condition;
  node_t consequent;
  node_t alternate;
};
typedef struct _cubec_expression_type_ternary_init_t cubec_expression_type_ternary_init_t;

/**
 * @brief Try to parse a ternary (conditional) type expression:
 *        condition ? type_expr_when_true : type_expr_when_false
 * @param allocator The allocator to use for memory allocation
 * @param tokens The token vector
 * @param position Current position in the token stream (updated on success)
 * @param filename The source filename for error reporting
 * @return A new cubec_expression_type_ternary_t node, or the condition-only
 *         base type expression if no '?' follows
 */
node_t read_expression_type_ternary(allocator_t allocator, vec_t tokens,
                                    size_t *position, const char *filename);

#ifdef __cplusplus
}
#endif
#endif
