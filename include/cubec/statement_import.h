#ifndef _H_CUBEC_CUBEC_STATEMENT_IMPORT_
#define _H_CUBEC_CUBEC_STATEMENT_IMPORT_
#include "core/emit_context.h"
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
 * @brief AST node for import statement.
 *
 * Syntax:
 *   import <module_name> from "<module_path>";
 *
 * Examples:
 *   import std from "std";
 *   import vec from "std/vec";
 *   import io from "./io";
 */
struct _cubec_statement_import_t;
struct _cubec_statement_import_t {
  struct _node_t super;
  node_t module_name; /**< Identifier node for the module name */
  node_t path;        /**< String literal node for the module path */
};
typedef struct _cubec_statement_import_t *cubec_statement_import_t;

extern type_t g_cubec_statement_import_type;

struct _cubec_statement_import_init_t {
  location_t location;
  node_t parent;
  node_t module_name;
  node_t path;
};
typedef struct _cubec_statement_import_init_t cubec_statement_import_init_t;

/**
 * @brief Try to parse an import statement: import <name> from "<path>" ;
 * @param allocator The allocator to use
 * @param tokens The token list
 * @param position Current position in token list (updated on success)
 * @param filename The source filename for error reporting
 * @return A new cubec_statement_import_t node, or NULL if current token
 *         is not 'import'.
 */
node_t read_statement_import(context_t ctx, vec_t tokens, size_t *position,
                             const char *filename);

node_t create_statement_import(context_t ctx, location_t loc,
                               const char *module_name, const char *path);

void write_statement_import(writer_t writer, node_t node);

void emit_statement_import(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
