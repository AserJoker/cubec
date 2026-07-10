#ifndef _H_CUBEC_CUBEC_STATEMENT_DECLARATION_TYPE_
#define _H_CUBEC_CUBEC_STATEMENT_DECLARATION_TYPE_
#include "core/allocator.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for `type` alias declaration.
 *
 * Syntax:
 *   type <name> [<generic_params>] = <type_expression> ;
 *
 * Examples:
 *   type MyInt = i32
 *   type Vec3[T] = Vec[Vec[Vec[T]]]
 *   type Pair[A, B] = struct { first: A, second: B }
 *   type Mutable[T] = RemoveConst[T]
 */
struct _cubec_statement_declaration_type_t;
struct _cubec_statement_declaration_type_t {
  struct _node_t super;
  bool is_export;    /**< Whether this declaration is exported */
  node_t name;       /**< Identifier node for the type alias name */
  vec_t params;      /**< Vector of identifier nodes for optional generic parameters (may be NULL) */
  node_t type_value; /**< Type expression parsed by read_expression_type */
};
typedef struct _cubec_statement_declaration_type_t *cubec_statement_declaration_type_t;

extern type_t g_cubec_statement_decltype;

struct _cubec_statement_declaration_type_init_t {
  location_t location;
  node_t parent;
  bool is_export;
  node_t name;
  vec_t params;
  node_t type_value;
};
typedef struct _cubec_statement_declaration_type_init_t cubec_statement_declaration_type_init_t;

/**
 * @brief Try to parse a type alias declaration: type <name> [<params>] = <type_expression> ;
 * @param allocator The allocator to use
 * @param tokens The token list
 * @param position Current position in token list (updated on success)
 * @param filename The source filename for error reporting
 * @return A new cubec_statement_declaration_type_t node, or NULL if current token
 *         is not 'type'.
 */
node_t read_statement_declaration_type(allocator_t allocator, vec_t tokens,
                                       size_t *position, const char *filename);

#ifdef __cplusplus
}
#endif
#endif
