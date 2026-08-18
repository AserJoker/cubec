#ifndef _H_CUBEC_CUBEC_EXPRESSION_CALL_
#define _H_CUBEC_CUBEC_EXPRESSION_CALL_
#include "core/emit_context.h"
#include "core/location.h"
#include "core/node.h"
#include "core/class.h"
#include "core/vec.h"
#include "cubec/expression.h"
#include "engine/vm.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _cubec_expression_call_t;
struct _cubec_expression_call_t {
  struct _cubec_expression_t super;
  node_t callee;   /**< The function being called */
  vec_t arguments; /**< vec_t of argument expression nodes (may contain
                         spread expressions) */
};
typedef struct _cubec_expression_call_t *cubec_expression_call_t;

extern class_t g_cubec_expression_call_class;

struct _cubec_expression_call_init_t {
  location_t location;
  node_t parent;
  node_t callee;
  vec_t arguments; /**< Already-parsed arguments vec (transferred to node) */
};
typedef struct _cubec_expression_call_init_t cubec_expression_call_init_t;

/**
 * @brief Try to parse a function-call expression: \c callee(arg1, arg2, ...)
 *
 * Called from \c read_value as a postfix operator after the callee has been
 * parsed. Checks for \c '(' at the current position, then parses a
 * comma-separated argument list until \c ')'.
 *
 * Each argument first tries \c read_expression_spread (to support \c ...expr
 * in function-call arguments), then falls back to \c read_expression.
 *
 * @param callee  The node already parsed as the function being called.
 *                Ownership is transferred to the new call node.
 * @return A new cubec_expression_call_t node, or NULL if the next token
 *         is not \c '(' (position is NOT advanced on NULL return).
 */
node_t read_expression_call(vm_t vm, vec_t tokens, size_t *position,
                            const char *filename, node_t callee);

node_t create_expression_call(vm_t vm, location_t loc, node_t callee,
                              vec_t args);


void emit_expression_call(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
