#ifndef _H_CUBEC_CUBEC_EXPRESSION_BINARY_
#define _H_CUBEC_CUBEC_EXPRESSION_BINARY_
#include "core/location.h"
#include "core/node.h"
#include "core/string.h"
#include "core/type.h"
#include "core/vec.h"
#include "core/writer.h"
#include "cubec/expression.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_expression_binary_t;
struct _cubec_expression_binary_t {
  struct _cubec_expression_t super;
  node_t left;  /**< Left operand (NULL for prefix unary) */
  node_t right; /**< Right operand */
  string_t opt; /**< Operator string (e.g. "!", "+", "-") */
};
typedef struct _cubec_expression_binary_t *cubec_expression_binary_t;

extern type_t g_cubec_expression_binary_type;

struct _cubec_expression_binary_init_t {
  location_t location;
  node_t parent;
  node_t left;
  node_t right;
  string_t opt;
};
typedef struct _cubec_expression_binary_init_t cubec_expression_binary_init_t;

/**
 * @brief Try to parse a prefix unary expression: \c !value, \c -value, etc.
 *        Supported operators: ! + - & * ~
 * @return A new cubec_expression_binary_t node (with left=NULL), or NULL if
 *         the current token is not a prefix operator.
 */
node_t read_expression_prefix(context_t ctx, vec_t tokens, size_t *position,
                              const char *filename);

/**
 * @brief Parse a binary expression with full operand-precedence parsing.
 *        Handles all binary infix operators respecting C precedence levels:
 *        || < && < | < ^ < & < == != < < > <= >= < << >> < + - < * / %
 *        Assignment and comma are NOT handled (not binary in cubec).
 * @return A cubec_expression_binary_t node for binary ops, or a unary/value
 *         node if no binary operators are present.
 */
node_t read_expression_binary(context_t ctx, vec_t tokens, size_t *position,
                              const char *filename);

node_t create_expression_binary(context_t ctx, location_t loc, const char *op,
                                node_t left, node_t right);

void write_expression_binary(writer_t writer, node_t node);

#ifdef __cplusplus
}
#endif
#endif
