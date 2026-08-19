#ifndef _H_CUBEC_ENGINE_AST_FUNC_
#define _H_CUBEC_ENGINE_AST_FUNC_
#include "core/allocator.h"
#include "core/class.h"
#include "core/node.h"
#include "engine/func.h"
#include "engine/callable_type.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _scope_t;

/**
 * @brief ast_func_t — AST function data object (extends func_t).
 *
 * Inherits func_t fields (func, name, closure_scope, root_scope) and adds:
 * - node: the AST declaration_function_t node for the function body
 * - template_scope: owned scope for generic type parameters (all own=false
 *   references; lifecycle managed by generic function cache). NULL for
 *   non-generic functions.
 *
 * The cfunction_t callback (_ast_func_call) is fixed for all ast_func_t
 * instances. It extracts the ast_func_t from value.data, retrieves the AST
 * node, and executes the function body via the runner layer.
 */
struct _ast_func_t {
  struct _func_t base;        /* inherited: func, name, closure_scope, root_scope */
  node_t node;                /* AST node: declaration_function_t (borrowed) */
  struct _scope_t *template_scope; /* owned: scope for generic params (own=false refs) */
};
typedef struct _ast_func_t *ast_func_t;

/** @brief Class descriptor for allocator_create. */
extern class_t g_ast_func_class;

/** @brief Init args for g_ast_func_class. */
typedef struct ast_func_init_t {
  cfunction_t func;             /* must be _ast_func_call */
  const char *name;             /* nullable, borrowed */
  struct _scope_t *closure_scope; /* nullable, owned */
  struct _scope_t *root_scope;    /* borrowed */
  node_t node;                  /* AST node (borrowed) */
  struct _scope_t *template_scope; /* nullable, owned */
} ast_func_init_t;

/** @brief Accessors */
node_t           ast_func_get_node(ast_func_t self);
struct _scope_t *ast_func_get_template_scope(ast_func_t self);

/**
 * @brief Create an AST function value.
 *
 * Creates an ast_func_t with _ast_func_call as the callback, registers it
 * in the current scope's cfuncs vec, and returns a callable value (own=false).
 *
 * @param vm     Virtual machine
 * @param ct     Callable type (defines the signature)
 * @param name   Function name (nullable, borrowed)
 * @param node   AST declaration_function_t node (borrowed)
 * @param template_scope Generic param scope (nullable, owned — transferred)
 * @return The callable value wrapping ast_func_t
 */
value_t create_ast_func_value(struct _vm_t *vm, callable_type_t ct,
                               const char *name, node_t node,
                               struct _scope_t *template_scope);

/**
 * @brief Check an AST function's body for type/syntax correctness (shadow=true).
 *
 * Executes the function body in shadow mode: no runtime side effects, but
 * type checking and validation are performed. Diagnostics are written to
 * vm_get_diagnostics(vm).
 *
 * @param vm       Virtual machine
 * @param callable The AST function callable value
 * @return void on success, exception on error
 */
value_t ast_func_check(struct _vm_t *vm, value_t callable);

/**
 * @brief Fixed cfunction_t callback for ast_func_t values.
 *
 * Called by the callable vtable when the function is invoked (shadow=false).
 * Extracts ast_func_t from value.data, sets up scope chain, and executes
 * the function body via _ast_func_exec(shadow=false).
 */
value_t _ast_func_call(struct _vm_t *vm, value_t fn, size_t argc,
                        value_t *argv);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_AST_FUNC_ */
