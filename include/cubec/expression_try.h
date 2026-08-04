#ifndef _H_CUBEC_CUBEC_EXPRESSION_TRY_
#define _H_CUBEC_CUBEC_EXPRESSION_TRY_
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
 * @brief Postfix try/unwrap expression: <host>.?
 *        On a union: checks if the accessed variant is active, returns its
 *        value or propagates the error.
 *        On a Result type: calls isError() / value() / error() for error
 *        propagation. On a pointer: auto-dereferences.
 */
struct _cubec_expression_try_t;
struct _cubec_expression_try_t {
  struct _cubec_expression_t super;
  node_t host; /**< The operand of .? */
};
typedef struct _cubec_expression_try_t *cubec_expression_try_t;

extern type_t g_cubec_expression_try_type;

struct _cubec_expression_try_init_t {
  location_t location;
  node_t parent;
  node_t host;
};
typedef struct _cubec_expression_try_init_t cubec_expression_try_init_t;

/**
 * @brief Try to parse a postfix try/unwrap expression: <host>.?
 * @param host The already-parsed left operand (the value before the operator)
 * @return A new cubec_expression_try_t node, or NULL if the current token
 *         is not '.' followed by '?'.
 */
node_t read_expression_try(context_t ctx, vec_t tokens, size_t *position,
                           const char *filename, node_t host);

node_t create_expression_try(context_t ctx, location_t loc, node_t host);

void write_expression_try(writer_t writer, node_t node);

void emit_expression_try(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
