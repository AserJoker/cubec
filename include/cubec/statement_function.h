#ifndef _H_CUBEC_CUBEC_STATEMENT_FUNCTION_
#define _H_CUBEC_CUBEC_STATEMENT_FUNCTION_
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for function declaration statement.
 *
 * Syntax:
 *   [export] [inline] func <name> [<generic_params>] (<params>) [:
 * <return_type>] { <body> } | ; [extern] func <name> [<generic_params>]
 * (<params>) [: <return_type>] ; [builtin] func <name> [<generic_params>]
 * (<params>) [: <return_type>] ; [comptime] func <name> [<generic_params>]
 * (<params>) [: <return_type>] { <body> }
 *
 * Modifiers:
 * - export: function is exported from the module (source-level, compatible with
 * inline/builtin/comptime)
 * - inline: function is inlined (like C static inline)
 * - extern: function has no body, declared for external linkage (mutually
 * exclusive with export/inline/builtin/comptime)
 * - builtin: function is compiler-provided, no body (mutually exclusive with
 * extern/comptime)
 * - comptime: function is compile-time evaluated, requires body (mutually
 * exclusive with extern/builtin)
 *
 * Return type is optional (omitted = void).
 * Body is optional (NULL for extern/interface functions, ending with ';').
 * C-style variadic '...' is only allowed in extern functions.
 *
 * Examples:
 *   func add(a: i32, b: i32): i32 { return a + b; }
 *   func greet(name: *u8) { }
 *   export func create(): Vec[i32] { }
 *   inline func helper(x: i32): i32 { return x * 2; }
 *   extern func read_file(path: *u8): []u8;
 *   extern func printf(fmt: *u8, ...): i32;
 *   builtin func panic(msg: []u8): void;
 *   comptime func fib(n: u64): u64 { }
 *   export comptime func MAX(a: i32, b: i32): i32 { }
 *   func[T](x: T): T { return x; }
 *   func next(self: *Iterator): Item;
 */
struct _cubec_statement_function_t;
struct _cubec_statement_function_t {
  struct _node_t super;
  bool is_export;    /**< Whether this function is exported */
  bool is_exportlib; /**< Whether this function is exported with C ABI (no
                        mangling) */
  bool is_inline;    /**< Whether this function is inline */
  bool is_extern;    /**< Whether this is an extern function (no body) */
  bool is_builtin;   /**< Whether this is a builtin function (compiler-provided,
                        no body) */
  bool is_comptime;  /**< Whether this is a comptime function (requires body) */
  bool is_c_variadic;   /**< Whether this function has C-style variadic '...' */
  node_t name;          /**< Identifier node for the function name */
  vec_t generic_params; /**< Vector of cubec_generic_param_t (may be NULL) */
  vec_t arguments;      /**< Vector of cubec_function_argument_t */
  node_t return_type;   /**< Return type expression (may be NULL = void) */
  node_t body; /**< Block statement node (may be NULL for extern/interface) */
  vec_t decorators; /**< Vector of cubec_decorator_t (may be NULL) */
  vec_t captures;   /**< Vector of cubec_function_capture_t (nullable,
                       auto_dispose) */
};
typedef struct _cubec_statement_function_t *cubec_statement_function_t;

extern type_t g_cubec_statement_function_type;

struct _cubec_statement_function_init_t {
  location_t location;
  node_t parent;
  bool is_export;
  bool is_exportlib;
  bool is_inline;
  bool is_extern;
  bool is_builtin;
  bool is_comptime;
  bool is_c_variadic;
  node_t name;
  vec_t generic_params;
  vec_t arguments;
  node_t return_type;
  node_t body;
  vec_t decorators;
  vec_t captures;
};
typedef struct _cubec_statement_function_init_t cubec_statement_function_init_t;

/**
 * @brief Try to parse a function declaration statement.
 * @param allocator The allocator to use.
 * @param tokens The token list.
 * @param position Current position in token list (updated on success).
 * @param filename The source filename for error reporting.
 * @return A new cubec_statement_function_t node, or NULL if current token
 *         is not a function declaration prefix
 * (export/inline/extern/builtin/comptime/func).
 */
node_t read_statement_function(context_t ctx, vec_t tokens, size_t *position,
                               const char *filename);

node_t create_statement_func(context_t ctx, location_t loc, const char *name,
                             vec_t args, node_t return_type, node_t body,
                             bool is_export, bool is_inline, bool is_extern,
                             bool is_builtin, bool is_comptime,
                             bool is_c_variadic);

#ifdef __cplusplus
}
#endif
#endif
