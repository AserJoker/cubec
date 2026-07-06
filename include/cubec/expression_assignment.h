#ifndef _H_CUBEC_CUBEC_EXPRESSION_ASSIGNMENT_
#define _H_CUBEC_CUBEC_EXPRESSION_ASSIGNMENT_
#include "core/allocator.h"
#include "core/location.h"
#include "core/node.h"
#include "core/string.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/expression.h"
#include "cubec/expression_binary.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Assignment expression for simple assignment and compound assignment.
 *        - <lvalue> = <rvalue>  (simple assignment)
 *        - <lvalue> += <rvalue> (compound assignment)
 *        - <lvalue> -= <rvalue>
 *        - <lvalue> *= <rvalue>
 *        - <lvalue> /= <rvalue>
 *        - <lvalue> %= <rvalue>
 *        - <lvalue> &= <rvalue>
 *        - <lvalue> |= <rvalue>
 *        - <lvalue> ^= <rvalue>
 *        - <lvalue> <<= <rvalue>
 *        - <lvalue> >>= <rvalue>
 *
 *        This reuses cubec_expression_binary_t struct with left=lvalue
 *        and right=rvalue, and opt containing the operator string.
 */
typedef cubec_expression_binary_t cubec_expression_assignment_t;

extern type_t g_cubec_expression_assignment_type;

struct _cubec_expression_assignment_init_t {
  location_t location;
  node_t parent;
  node_t lvalue;        /**< Left-hand side (must be an lvalue) */
  node_t rvalue;        /**< Right-hand side expression */
  string_t opt;         /**< Operator string (e.g. "=", "+=", "-=") */
};
typedef struct _cubec_expression_assignment_init_t
    cubec_expression_assignment_init_t;

/**
 * @brief Try to parse an assignment expression: <lvalue> <op> <rvalue>
 *        where <op> is =, +=, -=, *=, /=, %=, &=, |=, ^=, <<=, or >>=.
 *
 *        This function first reads a value as the potential lvalue, then
 *        checks if an assignment operator follows. If yes, it parses the
 *        rvalue expression and returns the assignment node. If no operator
 *        follows, it returns the value node as-is without creating an
 *        assignment node.
 *
 * @param allocator The allocator to use for memory allocation
 * @param tokens The token vector
 * @param position Current position in the token stream (updated on success)
 * @param filename The source filename for error reporting
 * @return A new cubec_expression_assignment_t node, or the value node if
 *         no assignment operator follows
 */
node_t read_expression_assignment(allocator_t allocator, vec_t tokens,
                                  size_t *position, const char *filename);

#ifdef __cplusplus
}
#endif
#endif