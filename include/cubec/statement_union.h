#ifndef _H_CUBEC_CUBEC_STATEMENT_UNION_
#define _H_CUBEC_CUBEC_STATEMENT_UNION_
#include "core/allocator.h"
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
 *   [export] union <name> [<generic_params>] { <fields> }
 *
 * Fields are union_field nodes with syntax: <name> : <type>
 * Fields are comma-separated.
 *
 * Examples:
 *   union Option[T] { value: T, tag: u64 }
 *   export union Result[T, E] { value: T, err: E }
 */
struct _cubec_statement_union_t;
struct _cubec_statement_union_t {
  struct _node_t super;
  bool is_export;       /**< Whether this union is exported */
  node_t name;          /**< Identifier node for the union name */
  vec_t generic_params; /**< Vector of cubec_generic_param_t (may be NULL) */
  vec_t fields;         /**< Vector of cubec_union_field_t (auto_dispose=true) */
};
typedef struct _cubec_statement_union_t *cubec_statement_union_t;

extern type_t g_cubec_statement_union_type;

struct _cubec_statement_union_init_t {
  location_t location;
  node_t parent;
  bool is_export;
  node_t name;
  vec_t generic_params;
  vec_t fields;
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
node_t read_statement_union(allocator_t allocator, vec_t tokens,
                             size_t *position, const char *filename);

#ifdef __cplusplus
}
#endif
#endif
