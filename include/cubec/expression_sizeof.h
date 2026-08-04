#ifndef _H_CUBEC_CUBEC_EXPRESSION_SIZEOF_
#define _H_CUBEC_CUBEC_EXPRESSION_SIZEOF_
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "core/emit_context.h"
#include "cubec/expression.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for sizeof expression (compile-time size computation).
 *
 * Syntax:
 *   sizeof(<expression>)
 *
 * Computes the size in bytes of the type of the inner expression at compile
 * time, without evaluating the expression. Returns a compile-time u64 value.
 *
 * Examples:
 *   sizeof(x)
 *   sizeof(a + b)
 *   sizeof(Vec[i32])
 */
struct _cubec_expression_sizeof_t;
struct _cubec_expression_sizeof_t {
  struct _cubec_expression_t super;
  node_t expression; /**< The expression whose type's size is computed */
};
typedef struct _cubec_expression_sizeof_t *cubec_expression_sizeof_t;

extern type_t g_cubec_expression_sizeof_type;

struct _cubec_expression_sizeof_init_t {
  location_t location;
  node_t parent;
  node_t expression;
};
typedef struct _cubec_expression_sizeof_init_t cubec_expression_sizeof_init_t;

/**
 * @brief Try to parse a sizeof expression.
 * @param allocator The allocator to use.
 * @param tokens The token list.
 * @param position Current position in token list (updated on success).
 * @param filename The source filename for error reporting.
 * @return A new cubec_expression_sizeof_t node, or NULL if current token
 *         is not 'sizeof' keyword.
 */
node_t read_expression_sizeof(context_t ctx, vec_t tokens, size_t *position,
                              const char *filename);

node_t create_expression_sizeof(context_t ctx, location_t loc, node_t expr);


void emit_expression_sizeof(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
