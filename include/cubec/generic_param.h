#ifndef _H_CUBEC_CUBEC_GENERIC_PARAM_
#define _H_CUBEC_CUBEC_GENERIC_PARAM_
#include "core/allocator.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for a single generic parameter in a declaration.
 *
 * Supports four forms defined in SKILL rules 1, 8:
 *   - Simple:      T
 *   - Constrained: T extends Numeric
 *   - Multi-constraint: T extends Printable & Serializable
 *   - Value:       N: u64
 *   - Rest:        ...T  (zero or more type args collected into T)
 *
 * Used by: type alias, func, struct, union, interface, builtin declarations.
 *
 * Examples:
 *   type Vec3[T] = Vec[Vec[Vec[T]]]
 *   type Pair[A, B] = struct { first: A, second: B }
 *   func[T extends Numeric](x: T) -> T
 *   func[N: u64, T](arr: [N]T) -> T
 *   type Variadic[...Args] = ...
 */
struct _cubec_generic_param_t;
struct _cubec_generic_param_t {
  struct _node_t super;
  node_t name;       /**< Literal identifier for the parameter name */
  vec_t constraints; /**< Optional vec of type expressions for `extends` constraint (may be NULL) */
  node_t value_type; /**< Optional type annotation for value generics like `N: u64` (may be NULL) */
  bool is_rest;      /**< Whether this is a rest param (prefixed with `...`) */
};
typedef struct _cubec_generic_param_t *cubec_generic_param_t;

extern type_t g_cubec_generic_param_type;

struct _cubec_generic_param_init_t {
  location_t location;
  node_t name;
  vec_t constraints;
  node_t value_type;
  bool is_rest;
};
typedef struct _cubec_generic_param_init_t cubec_generic_param_init_t;

/**
 * @brief Parse a comma-separated list of generic parameters:
 *        '[' <param> [, <param>]* ']'
 *
 * @param allocator The allocator to use.
 * @param tokens The token list.
 * @param position Current position in token list (updated on success).
 * @param filename The source filename for error reporting.
 * @return A vec_t of cubec_generic_param_t nodes with auto_dispose=true,
 *         or NULL if the current token is not '['.
 */
vec_t read_generic_params(allocator_t allocator, vec_t tokens,
                          size_t *position, const char *filename);

#ifdef __cplusplus
}
#endif
#endif
