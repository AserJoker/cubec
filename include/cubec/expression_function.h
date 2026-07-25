#ifndef _H_CUBEC_CUBEC_EXPRESSION_FUNCTION_
#define _H_CUBEC_CUBEC_EXPRESSION_FUNCTION_
#include "core/allocator.h"
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
 * @brief AST node for anonymous function expression (zero-time function).
 *
 * Syntax:
 *   func [|<captures>|] [<generic_params>] (<params>) [: <return_type>] { <body> }
 *
 * Captures use |...| delimiters. Each capture is a simple identifier.
 * Empty || can be omitted — no capture list means no captures.
 *
 * Examples:
 *   func () { }                    // no captures, no name
 *   func || () { }                 // explicit empty captures
 *   func |x, y| (a: i32): i32 { return x + y + a; }
 *   func |ctx| [T](x: T): T { return x; }
 *   func |x| (a: i32): i32 { return x + a; }(42)   // immediate call
 */
struct _cubec_expression_function_t;
struct _cubec_expression_function_t {
  struct _cubec_expression_t super;
  node_t name;            /**< Identifier node for named function (nullable, NULL for anonymous) */
  vec_t captures;         /**< Vector of cubec_function_capture_t (nullable, auto_dispose) */
  vec_t generic_params;   /**< Vector of cubec_generic_param_t (nullable, auto_dispose) */
  vec_t arguments;        /**< Vector of cubec_function_argument_t (auto_dispose) */
  node_t return_type;     /**< Return type expression (may be NULL = void) */
  node_t body;            /**< Block statement node (nullable for interface-style named funcs) */
  bool is_c_variadic;     /**< Whether this function has C-style variadic '...' */
};
typedef struct _cubec_expression_function_t *cubec_expression_function_t;

extern type_t g_cubec_expression_function_type;

struct _cubec_expression_function_init_t {
  location_t location;
  node_t parent;
  node_t name;
  vec_t captures;
  vec_t generic_params;
  vec_t arguments;
  node_t return_type;
  node_t body;
  bool is_c_variadic;
};
typedef struct _cubec_expression_function_init_t cubec_expression_function_init_t;

/**
 * @brief Try to parse an anonymous function expression.
 * @param allocator The allocator to use.
 * @param tokens The token list.
 * @param position Current position in token list (updated on success).
 * @param filename The source filename for error reporting.
 * @return A new cubec_expression_function_t node, or NULL if current token
 *         is not 'func' keyword.
 */
node_t read_expression_function(context_t ctx, vec_t tokens,
                                 size_t *position, const char *filename);

#ifdef __cplusplus
}
#endif
#endif
