#ifndef _H_CUBEC_CUBEC_STATEMENT_FUNCTION_
#define _H_CUBEC_CUBEC_STATEMENT_FUNCTION_
#include "core/location.h"
#include "core/node.h"
#include "core/class.h"
#include "core/vec.h"
#include "core/emit_context.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for function declaration statement (thin wrapper).
 *
 * This is a statement-level wrapper around declaration_function_t, mirroring
 * the statement_declaration_t -> declaration_variable_t pattern.
 * Statement-level concerns (is_export, is_exportlib, decorators) live here.
 * The actual function declaration details (name, params, body, etc.) are in
 * the declarator (declaration_function_t).
 *
 * Syntax:
 *   [decorators] [export|exportlib] [inline|extern|builtin|comptime]
 *   func <name> [<generic_params>] (<params>) [: <return_type>] { <body> } | ;
 */
struct _cubec_statement_function_t;
struct _cubec_statement_function_t {
  struct _node_t super;
  bool is_export;      /**< Whether this function is exported from the module */
  bool is_exportlib;   /**< Whether this function is exported with C ABI (no
                          mangling) */
  node_t declarator;   /**< Points to declaration_function_t */
  vec_t decorators;    /**< Vector of cubec_decorator_t (nullable, auto_dispose) */
};
typedef struct _cubec_statement_function_t *cubec_statement_function_t;

extern class_t g_cubec_statement_function_class;

struct _cubec_statement_function_init_t {
  location_t location;
  node_t parent;
  bool is_export;
  bool is_exportlib;
  node_t declarator;
  vec_t decorators;
};
typedef struct _cubec_statement_function_init_t cubec_statement_function_init_t;

/**
 * @brief Try to parse a function declaration statement.
 *
 * Parses optional modifiers (export/exportlib/inline/extern/builtin/comptime),
 * decorators, then delegates to read_declaration_function for the actual func
 * parsing, and wraps the result in a statement_function_t.
 *
 * @return A new cubec_statement_function_t node, or NULL if current token
 *         is not a function declaration prefix.
 */
node_t read_statement_function(context_t ctx, vec_t tokens, size_t *position,
                               const char *filename);

node_t create_statement_func(context_t ctx, location_t loc, const char *name,
                             vec_t args, node_t return_type, node_t body,
                             bool is_export, bool is_inline, bool is_extern,
                             bool is_builtin, bool is_comptime,
                             bool is_c_variadic, vec_t decorators);

void emit_statement_function(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
