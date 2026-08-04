#ifndef _H_CUBEC_CUBEC_EXPRESSION_ALIGNOF_
#define _H_CUBEC_CUBEC_EXPRESSION_ALIGNOF_
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "core/emit_context.h"
#include "core/writer.h"
#include "cubec/expression.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for alignof expression (compile-time alignment computation).
 *
 * Syntax:
 *   alignof(<expression>)
 *
 * Computes the alignment in bytes of the type of the inner expression at
 * compile time, without evaluating the expression. Returns a compile-time
 * u64 value.
 *
 * Examples:
 *   alignof(x)
 *   alignof(i32)
 *   alignof(Vec[i32])
 */
struct _cubec_expression_alignof_t;
struct _cubec_expression_alignof_t {
  struct _cubec_expression_t super;
  node_t expression; /**< The expression whose type's alignment is computed */
};
typedef struct _cubec_expression_alignof_t *cubec_expression_alignof_t;

extern type_t g_cubec_expression_alignof_type;

struct _cubec_expression_alignof_init_t {
  location_t location;
  node_t parent;
  node_t expression;
};
typedef struct _cubec_expression_alignof_init_t cubec_expression_alignof_init_t;

/**
 * @brief Try to parse an alignof expression.
 * @param allocator The allocator to use.
 * @param tokens The token list.
 * @param position Current position in token list (updated on success).
 * @param filename The source filename for error reporting.
 * @return A new cubec_expression_alignof_t node, or NULL if current token
 *         is not 'alignof' keyword.
 */
node_t read_expression_alignof(context_t ctx, vec_t tokens, size_t *position,
                               const char *filename);

node_t create_expression_alignof(context_t ctx, location_t loc, node_t expr);

void write_expression_alignof(writer_t writer, node_t node);

void emit_expression_alignof(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
