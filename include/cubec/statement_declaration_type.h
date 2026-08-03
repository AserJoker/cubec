#ifndef _H_CUBEC_CUBEC_STATEMENT_DECLARATION_TYPE_
#define _H_CUBEC_CUBEC_STATEMENT_DECLARATION_TYPE_
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
 * @brief AST node for `type` alias declaration.
 *
 * Syntax:
 *   [export] type <name> [<generic_params>] = <type_expression> ;
 *   [builtin] type <name> [<generic_params>] ;
 *
 * Modifiers:
 * - export: type is exported from the module (orthogonal with builtin)
 * - builtin: type is compiler-provided, no type_expression body
 *
 * For builtin type declarations, type_value is NULL and the '=' is not
 * required.
 *
 * Examples:
 *   type MyInt = i32;
 *   type Vec3[T] = Vec[Vec[Vec[T]]];
 *   export type Pair[A, B] = struct { first: A, second: B };
 *   builtin type RemoveConst[T extends const?];
 *   export builtin type Ptr[T];
 */
struct _cubec_statement_declaration_type_t;
struct _cubec_statement_declaration_type_t {
  struct _node_t super;
  bool is_export;  /**< Whether this declaration is exported */
  bool is_builtin; /**< Whether this is a builtin type (no type_value) */
  node_t name;     /**< Identifier node for the type alias name */
  vec_t params; /**< Vector of identifier nodes for optional generic parameters
                   (may be NULL) */
  node_t type_value; /**< Type expression parsed by read_expression_type (NULL
                        for builtin) */
  vec_t decorators;  /**< Vector of cubec_decorator_t (may be NULL) */
};
typedef struct _cubec_statement_declaration_type_t
    *cubec_statement_declaration_type_t;

extern type_t g_cubec_statement_decltype;

struct _cubec_statement_declaration_type_init_t {
  location_t location;
  node_t parent;
  bool is_export;
  bool is_builtin;
  node_t name;
  vec_t params;
  node_t type_value;
  vec_t decorators;
};
typedef struct _cubec_statement_declaration_type_init_t
    cubec_statement_declaration_type_init_t;

/**
 * @brief Try to parse a type alias declaration: [export|builtin] type <name>
 * [<params>] [= <type_expression>] ;
 * @param allocator The allocator to use
 * @param tokens The token list
 * @param position Current position in token list (updated on success)
 * @param filename The source filename for error reporting
 * @return A new cubec_statement_declaration_type_t node, or NULL if current
 * token is not a type declaration prefix (export/builtin/type).
 */
node_t read_statement_declaration_type(context_t ctx, vec_t tokens,
                                       size_t *position, const char *filename);

node_t create_statement_declaration_type(context_t ctx, location_t loc,
                                         const char *name, node_t type_value,
                                         bool is_export, bool is_builtin,
                                         vec_t decorators);

void write_statement_declaration_type(writer_t writer, node_t node);

#ifdef __cplusplus
}
#endif
#endif
