#ifndef _H_CUBEC_CUBEC_DECLARATION_CALLABLE_
#define _H_CUBEC_CUBEC_DECLARATION_CALLABLE_
#include "core/location.h"
#include "core/node.h"
#include "core/class.h"
#include "core/vec.h"
#include "cubec/expression.h"
#include "engine/vm.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief AST node for function type expression.
 *
 * Syntax:
 *   func(<type_list>) -> <return_type>
 *
 * Parameters are type-only (no names). Return type uses -> (not :)
 * to distinguish from function definitions which use :.
 *
 * Examples:
 *   func(i32, i32) -> i32      // two i32 params, returns i32
 *   func(*i32) -> void          // pointer param, returns void
 *   func() -> i32               // no params, returns i32
 *   func(i32, ...) -> void      // C-style variadic
 */
struct _cubec_declaration_callable_t;
struct _cubec_declaration_callable_t {
  struct _cubec_expression_t super;
  vec_t parameters;   /**< Vector of node_t type expressions (auto_dispose) */
  node_t return_type; /**< Return type expression (nullable = void) */
  bool is_c_variadic; /**< Whether this function type has C-style variadic '...'
                       */
};
typedef struct _cubec_declaration_callable_t *cubec_declaration_callable_t;

extern class_t g_cubec_declaration_callable_class;

struct _cubec_declaration_callable_init_t {
  location_t location;
  node_t parent;
  vec_t parameters;
  node_t return_type;
  bool is_c_variadic;
};
typedef struct _cubec_declaration_callable_init_t
    cubec_declaration_callable_init_t;

/**
 * @brief Try to parse a function type expression: func(type_list) -> type
 * @param allocator The allocator to use.
 * @param tokens The token list.
 * @param position Current position in token list (updated on success).
 * @param filename The source filename for error reporting.
 * @return A new cubec_declaration_callable_t node, or NULL if current token
 *         is not 'func' keyword.
 */
node_t read_declaration_callable(vm_t vm, vec_t tokens, size_t *position,
                                const char *filename);

node_t create_declaration_callable(vm_t vm, location_t loc,
                                  vec_t parameters, node_t return_type,
                                  bool is_c_variadic);

void emit_declaration_callable(emit_context_t ctx, node_t node);

#ifdef __cplusplus
}
#endif
#endif
