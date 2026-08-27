#include "engine/ast_defer.h"
#include "engine/scope.h"
#include "engine/vm.h"
#include "engine/void_type.h"
#include "engine/interrupt_type.h"
#include "run/run.h"
#include "cubec/statement_defer.h"

static void _ast_defer_init(void *self, allocator_t allocator, void *arg) {
  (void)allocator;
  ast_defer_t ad = (ast_defer_t)self;
  ast_defer_init_t *init = (ast_defer_init_t *)arg;

  /* init base (defer_t) — func is always NULL for ast_defer */
  ad->base.func = NULL;
  ad->base.closure_scope = init ? init->closure_scope : NULL;
  ad->base.root_scope = init ? init->root_scope : NULL;

  /* ast_defer_t fields */
  ad->node = init ? init->node : NULL;
  ad->template_scope = init ? init->template_scope : NULL;
}

static void _ast_defer_dispose(void *self, allocator_t allocator) {
  (void)allocator;
  ast_defer_t ad = (ast_defer_t)self;
  /* Disposing closure_scope also recursively disposes its children
   * (including template_scope). Both always exist. */
  if (ad->base.closure_scope) {
    scope_t cs = ad->base.closure_scope;
    ad->base.closure_scope = NULL;
    ad->template_scope = NULL; /* cleared to avoid dangling pointer */
    scope_dispose(cs);
  }
}

class_t g_ast_defer_class = {
    .size = sizeof(struct _ast_defer_t),
    .name = "cubec.engine.ast_defer",
    .init = (class_init_fn_t)_ast_defer_init,
    .dispose = (class_dispose_fn_t)_ast_defer_dispose,
    .clone = NULL,
    .move = NULL,
};

/* ---- Execution ---- */

value_t ast_defer_exec(vm_t vm, ast_defer_t ad) {
  if (!ad || !ad->node)
    return create_void_value(vm);

  cubec_statement_defer_t defer_node = (cubec_statement_defer_t)ad->node;
  if (!defer_node || !defer_node->body)
    return create_void_value(vm);

  /* Build scope chain: closure_scope -> template_scope -> [execution scopes]
   * Temporarily link closure->parent = root_scope for lookup chain,
   * then restore to NULL on exit (same pattern as _ast_func_exec). */
  scope_t closure = ad->base.closure_scope;
  scope_t saved_closure_parent = closure->parent;
  closure->parent = ad->base.root_scope;

  scope_t base = ad->template_scope;
  vm_set_scope(vm, base);

  /* Execute body (shadow=false; defers always execute at runtime) */
  value_t result = run_statement_block(vm, defer_node->body, false);

  /* Clean up runtime scopes created during body execution.
   * Defer body should not produce interrupts (shadow check prevents this),
   * but defensively clean up any leftover scopes. */
  while (vm_get_current_scope(vm) != base)
    vm_pop_scope(vm);

  /* Restore closure parent (isolated scope) */
  closure->parent = saved_closure_parent;

  return result;
}
