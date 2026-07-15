#ifndef _H_CUBEC_CUBEC_STATEMENT_INTERFACE_
#define _H_CUBEC_CUBEC_STATEMENT_INTERFACE_
#include "core/allocator.h"
#include "core/location.h"
#include "core/node.h"
#include "core/type.h"
#include "core/vec.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for interface declaration statement.
 *
 * Syntax:
 *   [export] interface <name> [<generic_params>] { <members> }
 *
 * Modifiers:
 * - export: interface is exported from the module
 *
 * Members inside the body:
 * - Associated type: type <name> [<generic_params>] ;
 * - Method signature: func <name> [<generic_params>] (<params>) [: <return_type>] ;
 *
 * Members are stored as a vec_t of mixed node types:
 * - cubec_statement_declaration_type_t (for associated type declarations)
 * - cubec_interface_method_t (for method signatures)
 *
 * Examples:
 *   interface Iterator {
 *       type Item;
 *       func next(self): Item;
 *   }
 *   interface Container[T] {
 *       func len(self): u64;
 *       func get(self, idx: u64): T;
 *   }
 *   export interface Serializable {
 *       func serialize(self): []u8;
 *   }
 */
struct _cubec_statement_interface_t;
struct _cubec_statement_interface_t {
  struct _node_t super;
  bool is_export;       /**< Whether this interface is exported */
  node_t name;          /**< Identifier node for the interface name */
  vec_t generic_params; /**< Vector of cubec_generic_param_t (may be NULL) */
  vec_t members;        /**< Vector of member nodes (auto_dispose=true) */
  vec_t decorators;     /**< Vector of cubec_decorator_t (may be NULL) */
};
typedef struct _cubec_statement_interface_t *cubec_statement_interface_t;

extern type_t g_cubec_statement_interface_type;

struct _cubec_statement_interface_init_t {
  location_t location;
  node_t parent;
  bool is_export;
  node_t name;
  vec_t generic_params;
  vec_t members;
  vec_t decorators;
};
typedef struct _cubec_statement_interface_init_t cubec_statement_interface_init_t;

/**
 * @brief Try to parse an interface declaration statement.
 * @param allocator The allocator to use.
 * @param tokens The token list.
 * @param position Current position in token list (updated on success).
 * @param filename The source filename for error reporting.
 * @return A new cubec_statement_interface_t node, or NULL if current token
 *         is not an interface declaration prefix (export/interface).
 */
node_t read_statement_interface(allocator_t allocator, vec_t tokens,
                                 size_t *position, const char *filename);

#ifdef __cplusplus
}
#endif
#endif
