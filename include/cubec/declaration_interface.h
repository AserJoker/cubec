#ifndef _H_CUBEC_CUBEC_DECLARATION_INTERFACE_
#define _H_CUBEC_CUBEC_DECLARATION_INTERFACE_
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
 * @brief AST node for anonymous interface type expression.
 *
 * Syntax:
 *   interface [<generic_params>] { <members> }
 *
 * Used as a type expression in parameter types, return types, variable types, etc.
 * Members follow the same rules as named interface declarations:
 * - Associated type: type <name> [<generic_params>] ;
 * - Method signature: func <name> [<generic_params>] (<params>) [: <return_type>] ;
 *
 * This is the expression form — it has no name and no is_export flag.
 * It inherits from expression_t (not node_t) so it participates in the
 * expression/type expression hierarchy.
 *
 * Examples:
 *   interface { func next(self): Item; }
 *   interface { type Item; func next(self): Item; }
 *   interface[T] { func get(self, idx: u64): T; }
 *   func foo(x: interface { func bar(): i32; }): void;
 *   var handler: interface { func run(); } = ...;
 */
struct _cubec_declaration_interface_t;
struct _cubec_declaration_interface_t {
  struct _cubec_expression_t super;
  vec_t generic_params; /**< Vector of cubec_generic_param_t (may be NULL) */
  vec_t members;        /**< Vector of member nodes (auto_dispose=true) */
};
typedef struct _cubec_declaration_interface_t *cubec_declaration_interface_t;

extern type_t g_cubec_declaration_interface_type;

struct _cubec_declaration_interface_init_t {
  location_t location;
  node_t parent;
  vec_t generic_params;
  vec_t members;
};
typedef struct _cubec_declaration_interface_init_t cubec_declaration_interface_init_t;

/**
 * @brief Try to parse an anonymous interface type expression.
 * @param allocator The allocator to use.
 * @param tokens The token list.
 * @param position Current position in token list (updated on success).
 * @param filename The source filename for error reporting.
 * @return A new cubec_declaration_interface_t node, or NULL if current token
 *         is not 'interface' keyword.
 */
node_t read_declaration_interface(context_t ctx, vec_t tokens,
                                       size_t *position, const char *filename);

/**
 * @brief Parse interface body after 'interface' keyword has been consumed.
 *
 * Parses [generic_params] { members } and returns an declaration_interface
 * node. Used by read_statement_interface for delegation — the statement parser
 * consumes 'export' + 'interface' + name, then delegates body parsing here.
 *
 * @param allocator The allocator to use.
 * @param tokens The token list.
 * @param position Current position in token list (updated on success).
 * @param filename The source filename for error reporting.
 * @param start_location Location of the 'interface' keyword (for error span).
 * @return A new cubec_declaration_interface_t node, or NULL on error.
 */
node_t read_declaration_interface_body(context_t ctx, vec_t tokens,
                                            size_t *position, const char *filename,
                                            location_t start_location);

node_t create_declaration_interface(context_t ctx, location_t loc,
                                        vec_t generic_params, vec_t members);


void emit_declaration_interface(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
