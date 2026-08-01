#ifndef _H_CUBEC_CUBEC_FUNCTION_CAPTURE_
#define _H_CUBEC_CUBEC_FUNCTION_CAPTURE_
#include "core/allocator.h"
#include "engine/context.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for a single capture item in an anonymous function.
 *
 * Syntax: <identifier>
 *
 * Example:
 *   x  (variable capture by name)
 */
struct _cubec_function_capture_t;
struct _cubec_function_capture_t {
  struct _node_t super;
  node_t identifier;  /**< Literal identifier for the capture name */
};
typedef struct _cubec_function_capture_t *cubec_function_capture_t;

extern type_t g_cubec_function_capture_type;

struct _cubec_function_capture_init_t {
  location_t location;
  node_t identifier;
};
typedef struct _cubec_function_capture_init_t cubec_function_capture_init_t;

/**
 * @brief Parse a single capture item: <identifier>
 * @param allocator The allocator to use.
 * @param tokens The token list.
 * @param position Current position in token list (updated on success).
 * @param filename The source filename for error reporting.
 * @return A new cubec_function_capture_t node, or NULL if current token
 *         is not an identifier.
 */
node_t read_function_capture(context_t ctx, vec_t tokens,
                              size_t *position, const char *filename);

node_t cubec_ast_create_func_capture(context_t ctx, location_t loc, const char *name);

#ifdef __cplusplus
}
#endif
#endif
