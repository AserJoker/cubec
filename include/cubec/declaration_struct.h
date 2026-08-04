#ifndef _H_CUBEC_CUBEC_DECLARATION_STRUCT_
#define _H_CUBEC_CUBEC_DECLARATION_STRUCT_
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
 * @brief AST node for anonymous struct type expression.
 *
 * Syntax:
 *   struct [<generic_params>] { <members> }
 *
 * Used as a type expression in type aliases, parameter types, etc.
 * Members are a mix of:
 * - Instance fields: [pub] <name> : <type> ;
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
 *   struct { var x: i32; var y: i32; }
 *   struct { x: f64; y: f64; }
 *   struct[T] { var data: *T; var len: u64; }
 *   type Pair[A, B] = struct { first: A; second: B; }
 */
struct _cubec_declaration_struct_t;
struct _cubec_declaration_struct_t {
  struct _cubec_expression_t super;
  vec_t generic_params; /**< Vector of cubec_generic_param_t (may be NULL) */
  vec_t members;        /**< Vector of member nodes (auto_dispose=true) */
};
typedef struct _cubec_declaration_struct_t *cubec_declaration_struct_t;

extern type_t g_cubec_declaration_struct_type;

struct _cubec_declaration_struct_init_t {
  location_t location;
  node_t parent;
  vec_t generic_params;
  vec_t members;
};
typedef struct _cubec_declaration_struct_init_t cubec_declaration_struct_init_t;

/**
 * @brief Try to parse an anonymous struct type expression.
 * @param allocator The allocator to use.
 * @param tokens The token list.
 * @param position Current position in token list (updated on success).
 * @param filename The source filename for error reporting.
 * @return A new cubec_declaration_struct_t node, or NULL if current token
 *         is not 'struct' keyword.
 */
node_t read_declaration_struct(context_t ctx, vec_t tokens,
                                    size_t *position, const char *filename);

/**
 * @brief Parse struct body after 'struct' keyword has been consumed.
 *
 * Parses [generic_params] { members } and returns an declaration_struct
 * node. Used by read_statement_struct for delegation.
 *
 * @param allocator The allocator to use.
 * @param tokens The token list.
 * @param position Current position in token list (updated on success).
 * @param filename The source filename for error reporting.
 * @param start_location Location of the 'struct' keyword (for error span).
 * @return A new cubec_declaration_struct_t node, or NULL on error.
 */
node_t read_declaration_struct_body(context_t ctx, vec_t tokens,
                                         size_t *position, const char *filename,
                                         location_t start_location,
                                         vec_t *out_implements);

node_t create_declaration_struct(context_t ctx, location_t loc,
                                     vec_t generic_params, vec_t members);


void emit_declaration_struct(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
