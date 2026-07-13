#ifndef _H_CUBEC_CUBEC_STATEMENT_DECLARATION_
#define _H_CUBEC_CUBEC_STATEMENT_DECLARATION_
#include "core/allocator.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for variable declaration statement.
 *
 * Syntax:
 *   [export] var <name> [: <type>] = <expression> ;
 *   [extern] var <name> [: <type>] ;
 *   [builtin] var <name> [: <type>] ;
 *
 * Modifiers:
 * - export: variable is exported from the module (orthogonal with builtin)
 * - extern: variable has external linkage, no initializer (mutually exclusive with export/builtin)
 * - builtin: variable is compiler-provided, no initializer (mutually exclusive with extern)
 *
 * Exactly one variable per statement. The declarator is a declaration_variable node
 * whose expression field is NULL for extern/builtin declarations.
 *
 * Examples:
 *   var x = 42;
 *   var x: i32 = 42;
 *   export var name: str = "hello";
 *   extern var errno: i32;
 *   builtin var VERSION: const str;
 */
struct _cubec_statement_declaration_t;
struct _cubec_statement_declaration_t {
  struct _node_t super;
  bool is_export;     /**< Whether this declaration is exported */
  bool is_extern;     /**< Whether this is an extern variable (no initializer) */
  bool is_builtin;    /**< Whether this is a builtin variable (no initializer) */
  node_t declarator;  /**< Single declaration_variable node */
};
typedef struct _cubec_statement_declaration_t *cubec_statement_declaration_t;

extern type_t g_cubec_statement_declaration_type;

struct _cubec_statement_declaration_init_t {
  location_t location;
  node_t parent;
  bool is_export;
  bool is_extern;
  bool is_builtin;
  node_t declarator;
};
typedef struct _cubec_statement_declaration_init_t cubec_statement_declaration_init_t;

/**
 * @brief Try to parse a declaration statement: [export|extern|builtin] var <declarator> ;
 * @param allocator The allocator to use
 * @param tokens The token list
 * @param position Current position in token list (updated on success)
 * @param filename The source filename for error reporting
 * @return A new cubec_statement_declaration_t node, or NULL if current token
 *         is not a declaration prefix (export/extern/builtin/var).
 */
node_t read_statement_declaration(allocator_t allocator, vec_t tokens,
                                  size_t *position, const char *filename);

#ifdef __cplusplus
}
#endif
#endif
