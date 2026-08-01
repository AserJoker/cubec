#ifndef _H_CUBEC_CUBEC_EXPRESSION_TYPE_FUNCTION_
#define _H_CUBEC_CUBEC_EXPRESSION_TYPE_FUNCTION_
#include "engine/context.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "cubec/expression.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for function type expression.
 *
 * Syntax:
 *   func(<type_list>) -> <return_type>
 *
 * Parameters are type-only (no names). Return type uses -> (not :)
 * to distinguish from function definitions which use :.
 *
 * Examples:
 *   func(i32, i32) -> i32      // two i32 params, returns i32
 *   func(*i32) -> void          // pointer param, returns void
 *   func() -> i32               // no params, returns i32
 *   func(i32, ...) -> void      // C-style variadic
 */
struct _cubec_expression_type_function_t;
struct _cubec_expression_type_function_t {
  struct _cubec_expression_t super;
  vec_t parameters;        /**< Vector of node_t type expressions (auto_dispose) */
  node_t return_type;      /**< Return type expression (nullable = void) */
  bool is_c_variadic;      /**< Whether this function type has C-style variadic '...' */
};
typedef struct _cubec_expression_type_function_t *cubec_expression_type_function_t;

extern type_t g_cubec_expression_type_function_type;

struct _cubec_expression_type_function_init_t {
  location_t location;
  node_t parent;
  vec_t parameters;
  node_t return_type;
  bool is_c_variadic;
};
typedef struct _cubec_expression_type_function_init_t cubec_expression_type_function_init_t;

/**
 * @brief Try to parse a function type expression: func(type_list) -> type
 * @param allocator The allocator to use.
 * @param tokens The token list.
 * @param position Current position in token list (updated on success).
 * @param filename The source filename for error reporting.
 * @return A new cubec_expression_type_function_t node, or NULL if current token
 *         is not 'func' keyword.
 */
node_t read_expression_type_function(context_t ctx, vec_t tokens,
                                     size_t *position, const char *filename);

#ifdef __cplusplus
}
#endif
#endif
