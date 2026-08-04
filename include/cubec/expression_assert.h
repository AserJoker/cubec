#ifndef _H_CUBEC_CUBEC_EXPRESSION_ASSERT_
#define _H_CUBEC_CUBEC_EXPRESSION_ASSERT_
#include "core/emit_context.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/expression.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Postfix assert/panic expression: <host>.!
 *        On a union: asserts that the accessed variant is active, panics
 *        otherwise. On a Result type: calls isError() / value() / error(),
 *        panicking on error. On a pointer: auto-dereferences.
 */
struct _cubec_expression_assert_t;
struct _cubec_expression_assert_t {
  struct _cubec_expression_t super;
  node_t host; /**< The operand of .! */
};
typedef struct _cubec_expression_assert_t *cubec_expression_assert_t;

extern type_t g_cubec_expression_assert_type;

struct _cubec_expression_assert_init_t {
  location_t location;
  node_t parent;
  node_t host;
};
typedef struct _cubec_expression_assert_init_t cubec_expression_assert_init_t;

/**
 * @brief Try to parse a postfix assert/panic expression: <host>.!
 * @param host The already-parsed left operand (the value before the operator)
 * @return A new cubec_expression_assert_t node, or NULL if the current
 *         token is not '.' followed by '!'.
 */
node_t read_expression_assert(context_t ctx, vec_t tokens, size_t *position,
                              const char *filename, node_t host);

node_t create_expression_assert(context_t ctx, location_t loc, node_t host);


void emit_expression_assert(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
