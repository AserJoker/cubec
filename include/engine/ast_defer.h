#ifndef _H_CUBEC_ENGINE_AST_DEFER_
#define _H_CUBEC_ENGINE_AST_DEFER_
#include "core/allocator.h"
#include "core/class.h"
#include "core/node.h"
#include "engine/defer.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif

struct _scope_t;

/**
 * @brief ast_defer_t — AST defer data object (extends defer_t).
 *
 * Inherits defer_t fields (func=NULL, closure_scope, root_scope) and adds:
 * - node: the AST statement_defer_t node for the defer body (borrowed)
 * - template_scope: owned scope as child of closure_scope, always present
 *   (empty for non-generic defers). Keeps scope chain consistent:
 *   closure_scope -> template_scope -> [execution scopes].
 *
 * Disposing closure_scope recursively disposes template_scope.
 */
struct _ast_defer_t {
  struct _defer_t base;            /* inherited: func(NULL), closure_scope, root_scope */
  node_t node;                     /* AST node: statement_defer_t (borrowed) */
  struct _scope_t *template_scope; /* owned (child of closure_scope) */
};
typedef struct _ast_defer_t *ast_defer_t;

/** @brief Class descriptor for allocator_create. */
extern class_t g_ast_defer_class;

/** @brief Init args for g_ast_defer_class. */
typedef struct ast_defer_init_t {
  struct _scope_t *closure_scope;  /* nullable, owned */
  struct _scope_t *root_scope;     /* borrowed */
  node_t node;                     /* AST node (borrowed) */
  struct _scope_t *template_scope; /* nullable, owned */
} ast_defer_init_t;

/**
 * @brief Execute an AST defer's body.
 *
 * Sets up the scope chain (closure_scope -> template_scope), executes the
 * defer body via run_statement_block, and cleans up runtime scopes.
 * Returns the result value (void on normal completion, interrupt/exception
 * on error — callers should check and handle accordingly).
 *
 * @param vm  Virtual machine
 * @param ad  AST defer to execute
 * @return Result value from the body execution
 */
struct _vm_t;
value_t ast_defer_exec(struct _vm_t *vm, ast_defer_t ad);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_AST_DEFER_ */
