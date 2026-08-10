#ifndef _H_CUBEC_CUBEC_DECLARATION_FUNCTION_
#define _H_CUBEC_CUBEC_DECLARATION_FUNCTION_
#include "core/location.h"
#include "core/node.h"
#include "core/class.h"
#include "core/vec.h"
#include "core/emit_context.h"
#include "cubec/declaration.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for function declaration (both named and anonymous).
 *
 * Syntax (named):
 *   func <name> [<generic_params>] (<params>) [: <return_type>] { <body> }
 *   inline func <name> ...
 *   extern func <name> ... ;
 *   builtin func <name> ... ;
 *   comptime func <name> ... { <body> }
 *
 * Syntax (anonymous):
 *   func [|<captures>|] [<generic_params>] (<params>) [: <return_type>] {
 * <body> }
 *
 * This node replaces the old expression_function_t. Named functions appear inside
 * a statement_function_t wrapper (which carries is_export, is_exportlib,
 * decorators). Anonymous functions appear directly in expression context.
 *
 * name is NULL for anonymous functions, non-NULL for named functions.
 * is_inline/is_extern/is_builtin/is_comptime are declaration-level modifiers.
 * is_export/is_exportlib are statement-level and live in statement_function_t.
 */
struct _cubec_declaration_function_t;
struct _cubec_declaration_function_t {
  struct _cubec_declaration_t super;
  node_t name;          /**< Identifier node (nullable, NULL for anonymous) */
  vec_t captures;       /**< Vector of cubec_function_capture_t (nullable,
                           auto_dispose) */
  vec_t generic_params; /**< Vector of cubec_generic_param_t (nullable,
                           auto_dispose) */
  vec_t arguments;      /**< Vector of cubec_function_argument_t (auto_dispose) */
  node_t return_type;   /**< Return type expression (nullable = void) */
  node_t body;          /**< Block statement node (nullable for extern/builtin) */
  bool is_inline;       /**< Whether this is an inline function */
  bool is_extern;       /**< Whether this is an extern function (no body) */
  bool is_builtin;      /**< Whether this is a builtin function (no body) */
  bool is_comptime;     /**< Whether this is a comptime function (requires body) */
  bool is_c_variadic;   /**< Whether this function has C-style variadic '...' */
};
typedef struct _cubec_declaration_function_t *cubec_declaration_function_t;

extern class_t g_cubec_declaration_function_class;

struct _cubec_declaration_function_init_t {
  location_t location;
  node_t parent;
  node_t name;
  vec_t captures;
  vec_t generic_params;
  vec_t arguments;
  node_t return_type;
  node_t body;
  bool is_inline;
  bool is_extern;
  bool is_builtin;
  bool is_comptime;
  bool is_c_variadic;
};
typedef struct _cubec_declaration_function_init_t
    cubec_declaration_function_init_t;

/**
 * @brief Parse a function declaration (func keyword + everything after).
 *
 * Parses: func [|captures|] [name] [generic_params] (params) [: return_type]
 * {body} or ; Used both for named top-level functions (called from
 * read_statement_function) and anonymous function expressions (called from
 * expression parser).
 *
 * @return A new declaration_function_t node, or NULL if 'func' keyword not
 * found.
 */
node_t read_declaration_function(context_t ctx, vec_t tokens, size_t *position,
                                const char *filename);

node_t create_declaration_function(context_t ctx, location_t loc, node_t name,
                                   vec_t captures, vec_t generic_params,
                                   vec_t args, node_t return_type, node_t body,
                                   bool is_inline, bool is_extern,
                                   bool is_builtin, bool is_comptime,
                                   bool is_c_variadic);

void emit_declaration_function(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
