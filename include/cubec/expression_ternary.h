#ifndef _H_CUBEC_CUBEC_EXPRESSION_TERNARY_
#define _H_CUBEC_CUBEC_EXPRESSION_TERNARY_
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/writer.h"
#include "cubec/expression.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_expression_ternary_t;
struct _cubec_expression_ternary_t {
  struct _cubec_expression_t super;
  node_t condition;  /**< Condition/test expression */
  node_t consequent; /**< Expression when condition is true */
  node_t alternate;  /**< Expression when condition is false */
};
typedef struct _cubec_expression_ternary_t *cubec_expression_ternary_t;

extern type_t g_cubec_expression_ternary_type;

struct _cubec_expression_ternary_init_t {
  location_t location;
  node_t parent;
  node_t condition;
  node_t consequent;
  node_t alternate;
};
typedef struct _cubec_expression_ternary_init_t cubec_expression_ternary_init_t;

/**
 * @brief Try to parse a ternary (conditional) expression: cond ? consequent :
 * alternate
 * @param allocator The allocator to use for memory allocation
 * @param tokens The token vector
 * @param position Current position in the token stream (updated on success)
 * @param filename The source filename for error reporting
 * @return A new cubec_expression_ternary_t node, or the condition if no '?'
 * follows
 */
node_t read_expression_ternary(context_t ctx, vec_t tokens, size_t *position,
                               const char *filename);

node_t create_expression_ternary(context_t ctx, location_t loc, node_t cond,
                                 node_t then_branch, node_t else_branch);

void write_expression_ternary(writer_t writer, node_t node);

#ifdef __cplusplus
}
#endif
#endif
