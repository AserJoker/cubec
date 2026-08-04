#ifndef _H_CUBEC_CUBEC_FUNCTION_ARGUMENT_
#define _H_CUBEC_CUBEC_FUNCTION_ARGUMENT_
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "core/emit_context.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for a single function parameter.
 *
 * Syntax: <identifier> [: <type>]
 *
 * Examples:
 *   a: i32
 *   self: *MyType
 *   x         (type omitted)
 */
struct _cubec_function_argument_t;
struct _cubec_function_argument_t {
  struct _node_t super;
  node_t identifier; /**< Literal identifier for the parameter name */
  node_t type;       /**< Type expression (may be NULL if omitted) */
  bool is_rest; /**< True if this parameter uses ... prefix (pack parameter) */
};
typedef struct _cubec_function_argument_t *cubec_function_argument_t;

extern type_t g_cubec_function_argument_type;

struct _cubec_function_argument_init_t {
  location_t location;
  node_t identifier;
  node_t type;
  bool is_rest;
};
typedef struct _cubec_function_argument_init_t cubec_function_argument_init_t;

/**
 * @brief Parse a single function parameter: <identifier> [: <type>]
 * @param allocator The allocator to use.
 * @param tokens The token list.
 * @param position Current position in token list (updated on success).
 * @param filename The source filename for error reporting.
 * @return A new cubec_function_argument_t node, or NULL if current token
 *         is not an identifier.
 */
node_t read_function_argument(context_t ctx, vec_t tokens, size_t *position,
                              const char *filename);

node_t create_function_argument(context_t ctx, location_t loc, const char *name,
                                node_t type);


void emit_function_argument(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
