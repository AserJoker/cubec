#ifndef _H_CUBEC_CUBEC_STATEMENT_DECLARATION_
#define _H_CUBEC_CUBEC_STATEMENT_DECLARATION_
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "core/writer.h"
#include "engine/context.h"
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
 *   [comptime] var <name> [: <type>] = <expression> ;
 *   [export] using <name> [: <type>] = <expression> ;
 *   using <name> [: <type>] = <expression> ;
 *   using <name> [: <type>] = undefined ;
 *
 * Modifiers:
 * - export: variable is exported from the module (orthogonal with
 * builtin/comptime)
 * - extern: variable has external linkage, no initializer (mutually exclusive
 * with export/builtin/comptime)
 * - builtin: variable is compiler-provided, no initializer (mutually exclusive
 * with extern/comptime)
 * - comptime: variable is compile-time evaluated, requires initializer
 * (mutually exclusive with extern/builtin)
 * - using: auto-defer __dispose__ at scope exit (mutually exclusive with
 * extern/builtin/comptime)
 *
 * Exactly one variable per statement. The declarator is a declaration_variable
 * node whose expression field is NULL for extern/builtin declarations.
 *
 * Examples:
 *   var x = 42;
 *   var x: i32 = 42;
 *   export var name: str = "hello";
 *   extern var errno: i32;
 *   builtin var VERSION: const str;
 *   comptime var PI: f64 = 3.14;
 *   export comptime var MAX: i32 = 1024;
 */
struct _cubec_statement_declaration_t;
struct _cubec_statement_declaration_t {
  struct _node_t super;
  bool is_export;    /**< Whether this declaration is exported */
  bool is_exportlib; /**< Whether this declaration is exported with C ABI (no
                        mangling) */
  bool is_extern;    /**< Whether this is an extern variable (no initializer) */
  bool is_builtin;   /**< Whether this is a builtin variable (no initializer) */
  bool is_comptime;  /**< Whether this is a comptime variable (requires
                        initializer) */
  bool is_using; /**< Whether this is a using variable (auto-defer __dispose__)
                  */
  node_t declarator; /**< Single declaration_variable node */
  vec_t decorators;  /**< Vector of cubec_decorator_t (may be NULL) */
};
typedef struct _cubec_statement_declaration_t *cubec_statement_declaration_t;

extern type_t g_cubec_statement_declaration_type;

struct _cubec_statement_declaration_init_t {
  location_t location;
  node_t parent;
  bool is_export;
  bool is_exportlib;
  bool is_extern;
  bool is_builtin;
  bool is_comptime;
  bool is_using;
  node_t declarator;
  vec_t decorators;
};
typedef struct _cubec_statement_declaration_init_t
    cubec_statement_declaration_init_t;

/**
 * @brief Try to parse a declaration statement:
 * [export|extern|builtin|comptime|using] var <declarator> ;
 * @param allocator The allocator to use
 * @param tokens The token list
 * @param position Current position in token list (updated on success)
 * @param filename The source filename for error reporting
 * @return A new cubec_statement_declaration_t node, or NULL if current token
 *         is not a declaration prefix (export/extern/builtin/comptime/var).
 */
node_t read_statement_declaration(context_t ctx, vec_t tokens, size_t *position,
                                  const char *filename);

node_t create_statement_declaration(context_t ctx, location_t loc,
                                    const char *name, node_t type, node_t expr,
                                    bool is_export, bool is_extern,
                                    bool is_builtin, bool is_comptime,
                                    bool is_using);

void write_statement_declaration(writer_t writer, node_t node);

#ifdef __cplusplus
}
#endif
#endif
