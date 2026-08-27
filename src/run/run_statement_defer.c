#include "run/run.h"
#include "engine/vm.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/interrupt_type.h"
#include "engine/diagnostic.h"
#include "engine/value.h"
#include "engine/scope.h"
#include "engine/name.h"
#include "engine/defer.h"
#include "engine/ast_defer.h"
#include "cubec/statement_defer.h"
#include "cubec/function_capture.h"
#include "cubec/literal_identifier.h"
#include "core/string.h"
#include "core/vec.h"

value_t run_statement_defer(vm_t vm, node_t node, bool shadow) {
  cubec_statement_defer_t defer_node = (cubec_statement_defer_t)node;
  allocator_t alloc = vm_get_allocator(vm);
  scope_t scope_before = vm_get_current_scope(vm);

  /* Shadow mode: type-check the defer body without registering it.
   * If the body produces an interrupt (return/break/continue), report a
   * diagnostic error — defer body must not contain control flow statements.
   * Also validate scope (same rules as script mode). */
  if (shadow) {
    /* Validate scope */
    switch (scope_before->kind) {
    case SCOPE_BLOCK:
    case SCOPE_FUNCTION:
    case SCOPE_FOR:
    case SCOPE_FOREACH:
    case SCOPE_CLOSURE:
      break; /* allowed */
    default:
      diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                           node->location,
                           "defer is not allowed in this scope");
      return create_void_value(vm);
    }
    if (defer_node->body) {
      value_t body_val = run_statement(vm, defer_node->body, true);

      if (value_is_interrupt(body_val)) {
        while (vm_get_current_scope(vm) != scope_before)
          vm_pop_scope(vm);
        diagnostic_list_push(vm_get_diagnostics(vm), DIAGNOSTIC_ERROR,
                             node->location,
                             "defer body must not contain return/break/continue");
        return create_void_value(vm);
      }

      if (value_is_abnormal(body_val)) {
        while (vm_get_current_scope(vm) != scope_before)
          vm_pop_scope(vm);
        return create_void_value(vm);
      }

      /* Clean up any scopes created during shadow check */
      while (vm_get_current_scope(vm) != scope_before)
        vm_pop_scope(vm);
    }
    return create_void_value(vm);
  }

  /* Script mode: create ast_defer_t, bind captures, push onto scope->defers */

  /* Validate scope: defer is only allowed in executable scopes
   * (block, function, for, foreach, closure). It is NOT allowed in
   * struct/union/interface/enum (SCOPE_TYPE), module (SCOPE_MODULE),
   * global (SCOPE_GLOBAL), or defer closure (SCOPE_DEFER) scopes. */
  switch (scope_before->kind) {
  case SCOPE_BLOCK:
  case SCOPE_FUNCTION:
  case SCOPE_FOR:
  case SCOPE_FOREACH:
  case SCOPE_CLOSURE:
    break; /* allowed */
  case SCOPE_TYPE:
    return create_exception_value(vm,
        "defer is not allowed in type scope");
  case SCOPE_MODULE:
    return create_exception_value(vm,
        "defer is not allowed in module scope");
  case SCOPE_GLOBAL:
    return create_exception_value(vm,
        "defer is not allowed in global scope");
  case SCOPE_DEFER:
    return create_exception_value(vm,
        "defer is not allowed in defer scope");
  default:
    return create_exception_value(vm,
        "defer is not allowed in this scope");
  }

  /* Create closure_scope: isolated scope (parent=NULL).
   * Parent is temporarily set to root_scope during execution. */
  scope_t closure = scope_create(alloc, SCOPE_DEFER, NULL, NULL);

  /* Create template_scope as child of closure (always present, keeps scope
   * chain consistent: closure -> template -> [execution scopes]). */
  scope_t template_scope = scope_create(alloc, SCOPE_TYPE, closure, NULL);

  /* Bind captures into closure_scope.
   * Each capture is cloned by value from the current scope chain, giving
   * the defer its own independent copy. Same pattern as _bind_captures
   * in run_declaration_function.c. */
  if (defer_node->captures) {
    size_t cap_count = vec_get_size(defer_node->captures);
    for (size_t i = 0; i < cap_count; i++) {
      cubec_function_capture_t cap =
          (cubec_function_capture_t)vec_get(defer_node->captures, i);
      if (!cap || !cap->identifier)
        continue;

      const char *cap_name = string_get(
          ((cubec_literal_identifier_t)cap->identifier)->value);

      /* Look up captured variable in current scope chain */
      name_t found = scope_lookup(vm_get_current_scope(vm), cap_name);
      if (!found || !found->ref) {
        scope_dispose(closure);
        return create_exception_value(vm,
            "defer capture '%s' not found in scope", cap_name);
      }

      /* Clone value into closure_scope */
      scope_t prev = vm_set_scope(vm, closure);
      value_t cloned = value_clone(vm, found->ref);
      vm_set_scope(vm, prev);

      if (value_is_abnormal(cloned)) {
        scope_dispose(closure);
        return cloned;
      }

      /* Bind name in closure_scope */
      name_t cn = name_create(closure->allocator, cloned);
      char *owned_name = cstring_clone(closure->allocator, cap_name);
      strmap_insert(closure->names, owned_name, cn);
      allocator_free(closure->allocator, &owned_name);
    }
  }

  /* Create ast_defer_t */
  ast_defer_init_t init = {
      .closure_scope = closure,
      .root_scope = vm_get_root_scope(vm),
      .node = node,
      .template_scope = template_scope,
  };
  ast_defer_t ad =
      (ast_defer_t)allocator_create(alloc, &g_ast_defer_class, &init);

  /* Push onto current scope's defers vec */
  vec_push(scope_before->defers, ad);

  return create_void_value(vm);
}
