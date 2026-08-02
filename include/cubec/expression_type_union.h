#ifndef _H_CUBEC_CUBEC_EXPRESSION_TYPE_UNION_
#define _H_CUBEC_CUBEC_EXPRESSION_TYPE_UNION_
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
 * @brief AST node for anonymous union type expression.
 *
 * Syntax:
 *   union [<generic_params>] { <members> }
 *
 * Members can be:
 * - Fields: <name> : <type>
 * - Static fields: var <name> [: <type>] = <expr> ;
 * - Associated types: type <name> [<params>] [= <type>] ;
 * - Methods: func <name> [<params>] (<args>) [: <return>] { <body> }
 * - Spread: ...<expr> ;
 * - Nested declarations
 *
 * This is the expression form — it has no name and no is_export flag.
 * It inherits from expression_t so it participates in the
 * expression/type expression hierarchy.
 *
 * Examples:
 *   union { value: i32, tag: u64 }
 *   union[T] { value: T, tag: u64 }
 *   type Res = union { ok: i32, err: *u8 }
 */
struct _cubec_expression_type_union_t;
struct _cubec_expression_type_union_t {
  struct _cubec_expression_t super;
  vec_t generic_params; /**< Vector of cubec_generic_param_t (may be NULL) */
  vec_t members;        /**< Vector of member nodes (auto_dispose=true) */
};
typedef struct _cubec_expression_type_union_t *cubec_expression_type_union_t;

extern type_t g_cubec_expression_type_union_type;

struct _cubec_expression_type_union_init_t {
  location_t location;
  node_t parent;
  vec_t generic_params;
  vec_t members;
};
typedef struct _cubec_expression_type_union_init_t cubec_expression_type_union_init_t;

/**
 * @brief Try to parse an anonymous union type expression.
 * @param allocator The allocator to use.
 * @param tokens The token list.
 * @param position Current position in token list (updated on success).
 * @param filename The source filename for error reporting.
 * @return A new cubec_expression_type_union_t node, or NULL if current token
 *         is not 'union' keyword.
 */
node_t read_expression_type_union(context_t ctx, vec_t tokens,
                                   size_t *position, const char *filename);

/**
 * @brief Parse union body after 'union' keyword has been consumed.
 *
 * Parses [generic_params] { fields } and returns an expression_type_union
 * node. Used by read_statement_union for delegation.
 *
 * @param allocator The allocator to use.
 * @param tokens The token list.
 * @param position Current position in token list (updated on success).
 * @param filename The source filename for error reporting.
 * @param start_location Location of the 'union' keyword (for error span).
 * @return A new cubec_expression_type_union_t node, or NULL on error.
 */
node_t read_expression_type_union_body(context_t ctx, vec_t tokens,
                                        size_t *position, const char *filename,
                                        location_t start_location,
                                        vec_t *out_implements);

node_t create_expression_type_union(context_t ctx, location_t loc,
                                    vec_t generic_params, vec_t members);

#ifdef __cplusplus
}
#endif
#endif
