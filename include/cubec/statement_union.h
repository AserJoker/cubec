#ifndef _H_CUBEC_CUBEC_STATEMENT_UNION_
#define _H_CUBEC_CUBEC_STATEMENT_UNION_
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
 * @brief AST node for union declaration statement.
 *
 * Syntax:
 *   [export] union <name> [<generic_params>] [implement <iface1, iface2, ...>] { <members> }
 *
 * Members can be:
 * - Fields: <name> : <type>
 * - Static fields: var <name> [: <type>] = <expr> ;
 * - Associated types: type <name> [<params>] [= <type>] ;
 * - Methods: func <name> [<params>] (<args>) [: <return>] { <body> }
 * - Spread: ...<expr> ;
 * - Nested declarations
 *
 * Examples:
 *   union Result[E, T] { value: T, error: E }
 *   export union Option[T] { value: T, empty: void }
 */
struct _cubec_statement_union_t;
struct _cubec_statement_union_t {
  struct _node_t super;
  bool is_export;       /**< Whether this union is exported */
  node_t name;          /**< Identifier node for the union name */
  vec_t generic_params; /**< Vector of cubec_generic_param_t (may be NULL) */
  vec_t implements;     /**< Vector of type expression nodes for implement clause (may be NULL) */
  vec_t members;        /**< Vector of member nodes (auto_dispose=true) */
  vec_t decorators;     /**< Vector of cubec_decorator_t (may be NULL) */
};
typedef struct _cubec_statement_union_t *cubec_statement_union_t;

extern type_t g_cubec_statement_union_type;

struct _cubec_statement_union_init_t {
  location_t location;
  node_t parent;
  bool is_export;
  node_t name;
  vec_t generic_params;
  vec_t implements;
  vec_t members;
  vec_t decorators;
};
typedef struct _cubec_statement_union_init_t cubec_statement_union_init_t;

/**
 * @brief Try to parse a union declaration statement.
 * @param allocator The allocator to use.
 * @param tokens The token list.
 * @param position Current position in token list (updated on success).
 * @param filename The source filename for error reporting.
 * @return A new cubec_statement_union_t node, or NULL if current token
 *         is not a union declaration prefix (export/union).
 */
node_t read_statement_union(context_t ctx, vec_t tokens,
                             size_t *position, const char *filename);

#ifdef __cplusplus
}
#endif
#endif
