#ifndef _H_CUBEC_CUBEC_EXPRESSION_SLICE_
#define _H_CUBEC_CUBEC_EXPRESSION_SLICE_
#include "core/location.h"
#include "core/node.h"
#include "core/class.h"
#include "core/emit_context.h"
#include "cubec/expression.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_expression_slice_t;
struct _cubec_expression_slice_t {
  struct _cubec_expression_t super;
  node_t host;   /**< The expression being sliced */
  node_t start;  /**< Start index expression (may be NULL if omitted) */
  node_t length; /**< Length expression (may be NULL if omitted) */
};
typedef struct _cubec_expression_slice_t *cubec_expression_slice_t;

extern class_t g_cubec_expression_slice_class;

struct _cubec_expression_slice_init_t {
  location_t location;
  node_t parent;
  node_t host;
  node_t start;
  node_t length;
};
typedef struct _cubec_expression_slice_init_t cubec_expression_slice_init_t;

/**
 * @brief Try to parse a slice expression: host[start:length]
 *
 * Called from read_value as a postfix operator after the host expression
 * has been parsed. Checks for '[' at the current position, then parses
 * optional start and length expressions separated by ':'.
 *
 * Format: host[start:length]
 *   - start is optional (if omitted, slice from beginning)
 *   - length is optional (if omitted, slice to end)
 *   - At least one of start or length must be present, or ':' must be present
 *
 * @param host The node already parsed as the expression being sliced.
 *             Ownership is transferred to the new slice node.
 * @return A new cubec_expression_slice_t node, or NULL if the next token
 *         is not '[' (position is NOT advanced on NULL return).
 */
node_t read_expression_slice(context_t ctx, vec_t tokens, size_t *position,
                             const char *filename, node_t host);

node_t create_expression_slice(context_t ctx, location_t loc, node_t host,
                               node_t start, node_t length);


void emit_expression_slice(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif