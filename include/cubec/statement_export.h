#ifndef _H_CUBEC_CUBEC_STATEMENT_EXPORT_
#define _H_CUBEC_CUBEC_STATEMENT_EXPORT_
#include "core/emit_context.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "engine/context.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for re-export statement.
 *
 * Syntax:
 *   export * from "<module_path>";
 *   export { name1, name2 } from "<module_path>";
 *
 * Examples:
 *   export * from "std/vec";
 *   export { Vec, Map } from "std/collections";
 */
struct _cubec_statement_export_t;
struct _cubec_statement_export_t {
  struct _node_t super;
  node_t path;  /**< String literal node for the module path */
  bool is_star; /**< true = export *, false = export { names } */
  vec_t names;  /**< vec of identifier nodes (NULL when is_star) */
};
typedef struct _cubec_statement_export_t *cubec_statement_export_t;

extern type_t g_cubec_statement_export_type;

struct _cubec_statement_export_init_t {
  location_t location;
  node_t parent;
  node_t path;
  bool is_star;
  vec_t names;
};
typedef struct _cubec_statement_export_init_t
    cubec_statement_export_init_t;

/**
 * @brief Try to parse a re-export statement: export * from "path" ;
 *        or export { a, b } from "path" ;
 * @param allocator The allocator to use
 * @param tokens The token list
 * @param position Current position in token list (updated on success)
 * @param filename The source filename for error reporting
 * @return A new cubec_statement_export_t node, or NULL if current token
 *         is not 'export' followed by '*' or '{'.
 */
node_t read_statement_export(context_t ctx, vec_t tokens, size_t *position,
                                  const char *filename);

node_t create_statement_export(context_t ctx, location_t loc, node_t path,
                                    bool is_star, vec_t names);


void emit_statement_export(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
