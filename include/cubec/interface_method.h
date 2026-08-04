#ifndef _H_CUBEC_CUBEC_INTERFACE_METHOD_
#define _H_CUBEC_CUBEC_INTERFACE_METHOD_
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#include "core/emit_context.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for an interface method signature.
 *
 * Syntax:
 *   func <name> [<generic_params>] (<params>) [: <return_type>] ;
 *
 * Used inside interface body. Method signatures have no body (end with ';').
 * Return type is optional (omitted = void).
 *
 * Examples:
 *   func next(self): Item;
 *   func len(self): u64;
 *   func get(self, idx: u64): T;
 *   func serialize(self): []u8;
 *   func identity[T](x: T): T;
 *   func bar();
 */
struct _cubec_interface_method_t;
struct _cubec_interface_method_t {
  struct _node_t super;
  node_t name;          /**< Identifier node for the method name */
  vec_t generic_params; /**< Vector of cubec_generic_param_t (may be NULL) */
  vec_t arguments;      /**< Vector of cubec_function_argument_t */
  node_t return_type;   /**< Return type expression (may be NULL = void) */
};
typedef struct _cubec_interface_method_t *cubec_interface_method_t;

extern type_t g_cubec_interface_method_type;

struct _cubec_interface_method_init_t {
  location_t location;
  node_t name;
  vec_t generic_params;
  vec_t arguments;
  node_t return_type;
};
typedef struct _cubec_interface_method_init_t cubec_interface_method_init_t;

/**
 * @brief Try to parse an interface method signature.
 * @param allocator The allocator to use.
 * @param tokens The token list.
 * @param position Current position in token list (updated on success).
 * @param filename The source filename for error reporting.
 * @return A new cubec_interface_method_t node, or NULL if current token
 *         is not a 'func' keyword.
 */
node_t read_interface_method(context_t ctx, vec_t tokens, size_t *position,
                             const char *filename);

node_t create_interface_method(context_t ctx, location_t loc, const char *name,
                               vec_t args, node_t return_type);


void emit_interface_method(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
