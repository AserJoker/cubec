#include "engine/ast_func.h"
#include "engine/callable_type.h"
#include "engine/func.h"
#include "engine/vm.h"
#include "engine/scope.h"
#include "engine/value.h"
#include "engine/void_type.h"
#include "engine/exception_type.h"
#include "engine/interrupt_type.h"
#include "engine/name.h"
#include "core/string.h"
#include "core/vec.h"
#include "run/run.h"
#include "cubec/declaration_function.h"
#include "cubec/function_argument.h"
#include "cubec/literal_identifier.h"

/* ---- Class implementation ---- */

static void _ast_func_init(void *self, allocator_t allocator, void *arg) {
  (void)allocator;
  ast_func_t af = (ast_func_t)self;
  ast_func_init_t *init = (ast_func_init_t *)arg;

  /* init base (func_t) */
  af->base.func = init ? init->func : NULL;
  af->base.name = init ? init->name : NULL;
  af->base.closure_scope = init ? init->closure_scope : NULL;
  af->base.root_scope = init ? init->root_scope : NULL;

  /* ast_func_t fields */
  af->node = init ? init->node : NULL;
  af->template_scope = init ? init->template_scope : NULL;
}

static void _ast_func_dispose(void *self, allocator_t allocator) {
  (void)allocator;
  ast_func_t af = (ast_func_t)self;
  /* base closure_scope is disposed by func_t class — but since we're using
   * our own class, we must do it here. */
  if (af->base.closure_scope) {
    scope_t cs = af->base.closure_scope;
    af->base.closure_scope = NULL;
    scope_dispose(cs);
  }
  /* template_scope is owned; dispose it */
  if (af->template_scope) {
    scope_t ts = af->template_scope;
    af->template_scope = NULL;
    scope_dispose(ts);
  }
  /* func pointer and name are borrowed, node is borrowed */
}

static void _ast_func_clone(void *self, allocator_t allocator, void *another) {
  (void)allocator;
  ast_func_t dst = (ast_func_t)self;
  ast_func_t src = (ast_func_t)another;
  dst->base.func = src->base.func;
  dst->base.name = src->base.name;
  dst->base.closure_scope = NULL; /* closures are unique */
  dst->base.root_scope = src->base.root_scope;
  dst->node = src->node;
  dst->template_scope = NULL; /* template scope not cloned */
}

class_t g_ast_func_class = {
    .size = sizeof(struct _ast_func_t),
    .name = "cubec.engine.ast_func",
    .init = (class_init_fn_t)_ast_func_init,
    .dispose = (class_dispose_fn_t)_ast_func_dispose,
    .clone = (class_clone_fn_t)_ast_func_clone,
    .move = NULL,
};

/* ---- Accessors ---- */

node_t ast_func_get_node(ast_func_t self) { return self->node; }

scope_t ast_func_get_template_scope(ast_func_t self) {
  return self->template_scope;
}

/* ---- Value constructor ---- */

value_t create_ast_func_value(vm_t vm, callable_type_t ct, const char *name,
                               node_t node, scope_t template_scope) {
  allocator_t alloc = vm_get_allocator(vm);
  ast_func_init_t init = {
      .func = _ast_func_call,
      .name = name,
      .closure_scope = NULL,
      .root_scope = vm_get_root_scope(vm),
      .node = node,
      .template_scope = template_scope,
  };
  ast_func_t af =
      (ast_func_t)allocator_create(alloc, &g_ast_func_class, &init);

  /* value.data = ast_func_t (borrowed ref), scope->cfuncs owns the lifecycle */
  value_t v = value_create(alloc, (type_t)ct, af, false);
  scope_t scope = vm_get_current_scope(vm);
  if (scope) {
    vec_push(scope->cfuncs, af);
    vec_push(scope->values, v);
  }
  return v;
}

/* ---- Internal execution engine ---- */

/**
 * @brief Unified AST function execution (both check and call).
 *
 * Scope chain: closure_scope → [template_scope] → arguments_scope → body_scope
 *
 * @param vm     Virtual machine
 * @param fn     The callable value (for self-inspection)
 * @param argc   Number of arguments
 * @param argv   Argument values (already safe_cast by callable vtable)
 * @param shadow true for check (type-only), false for call (runtime)
 * @return Result value
 */
static value_t _ast_func_exec(vm_t vm, value_t fn, size_t argc,
                               value_t *argv, bool shadow) {
  ast_func_t af = (ast_func_t)value_get_data(fn);
  if (!af || !af->node)
    return create_exception_value(vm, "ast_func: no AST node");

  callable_type_t ct = (callable_type_t)value_get_type(fn);

  /* ---- Save caller state ---- */
  scope_t caller_scope = vm_get_current_scope(vm);
  scope_t prev_root = vm_set_root_scope(vm, af->base.root_scope);
  /* vm_set_scope returns previous current_scope, but we already saved it
   * as caller_scope.  Set current to closure_scope (or root_scope if none). */
  scope_t entry_scope = af->base.closure_scope
                            ? af->base.closure_scope
                            : af->base.root_scope;
  vm_set_scope(vm, entry_scope);

  /* ---- Push template_scope (generic params) if present ---- */
  if (af->template_scope)
    vm_push_scope(vm, af->template_scope);

  /* ---- Create arguments scope and bind parameters ---- */
  allocator_t alloc = vm_get_allocator(vm);
  scope_t args_scope =
      scope_create(alloc, SCOPE_FUNCTION, vm_get_current_scope(vm), NULL);
  vm_push_scope(vm, args_scope);

  /* Extract parameter info from AST */
  cubec_declaration_function_t decl = (cubec_declaration_function_t)af->node;
  vec_t params = decl->arguments; /* vec of cubec_function_argument_t */
  uint64_t param_count = params ? vec_get_size(params) : 0;

  /* Clone each argument into args_scope with the parameter name */
  for (uint64_t i = 0; i < param_count && i < argc; i++) {
    cubec_function_argument_t param =
        (cubec_function_argument_t)vec_get(params, (size_t)i);
    if (!param || !param->identifier)
      continue;

    /* Extract parameter name from identifier node */
    cubec_literal_identifier_t ident =
        (cubec_literal_identifier_t)param->identifier;
    const char *param_name = string_get(ident->value);

    /* Clone the argument value into the current scope (args_scope) */
    scope_t prev = vm_set_scope(vm, args_scope);
    value_t cloned = value_clone(vm, argv[(size_t)i]);
    vm_set_scope(vm, prev);

    /* Bind name in args_scope */
    name_t n = name_create(args_scope->allocator, cloned);
    char *owned_name = cstring_clone(args_scope->allocator, param_name);
    strmap_insert(args_scope->names, owned_name, n);
    allocator_free(args_scope->allocator, &owned_name);
  }

  /* ---- Execute function body ---- */
  node_t body = decl->body;
  value_t result;
  if (body) {
    result = run_statement_block(vm, body, shadow);
  } else {
    /* No body (extern/builtin stub) — return void */
    result = create_void_value(vm);
  }

  /* ---- Process return value ---- */
  value_t return_value;

  if (value_is_interrupt(result)) {
    /* Interrupt (return statement): extract value, safe_cast, clone into
     * caller scope, then pop scopes back to entry_scope */
    value_t inner = interrupt_get_value(result);

    /* safe_cast to declared return type */
    type_t ret_type = callable_type_get_return_type(ct);
    inner = value_safe_cast(vm, inner, ret_type);
    if (value_is_abnormal(inner)) {
      /* safe_cast failed — treat as exception */
      vm_set_scope(vm, caller_scope);
      vm_set_root_scope(vm, prev_root);
      scope_t prev_s = vm_set_scope(vm, caller_scope);
      return_value = value_clone(vm, inner);
      vm_set_scope(vm, prev_s);
      return return_value;
    }

    /* Clone into caller scope */
    scope_t prev_s = vm_set_scope(vm, caller_scope);
    return_value = value_clone(vm, inner);
    vm_set_scope(vm, prev_s);

    /* Pop scopes back to entry_scope */
    while (vm_get_current_scope(vm) != entry_scope)
      vm_pop_scope(vm);

    /* Restore caller state */
    vm_set_scope(vm, caller_scope);
    vm_set_root_scope(vm, prev_root);
    return return_value;
  }

  if (value_is_abnormal(result)) {
    /* Exception: restore scope directly (no pop), clone exception into
     * caller scope */
    vm_set_scope(vm, caller_scope);
    vm_set_root_scope(vm, prev_root);
    /* Clone exception into caller scope */
    scope_t prev_s = vm_set_scope(vm, caller_scope);
    return_value = value_clone(vm, result);
    vm_set_scope(vm, prev_s);
    return return_value;
  }

  /* Void result (function body completed without return) */
  {
    type_t ret_type = callable_type_get_return_type(ct);
    if (type_get_kind(ret_type) != TYPE_KIND_VOID) {
      /* Non-void function missing return — error */
      vm_set_scope(vm, caller_scope);
      vm_set_root_scope(vm, prev_root);
      return create_exception_value(
          vm, "non-void function '%s' missing return statement",
          af->base.name ? af->base.name : "<anonymous>");
    }

    /* Clone void into caller scope */
    scope_t prev_s = vm_set_scope(vm, caller_scope);
    return_value = value_clone(vm, result);
    vm_set_scope(vm, prev_s);

    /* Pop scopes back to entry_scope */
    while (vm_get_current_scope(vm) != entry_scope)
      vm_pop_scope(vm);

    /* Restore caller state */
    vm_set_scope(vm, caller_scope);
    vm_set_root_scope(vm, prev_root);
    return return_value;
  }
}

/* ---- cfunction_t callback (shadow=false) ---- */

value_t _ast_func_call(vm_t vm, value_t fn, size_t argc, value_t *argv) {
  return _ast_func_exec(vm, fn, argc, argv, false);
}

/* ---- Public check API (shadow=true) ---- */

value_t ast_func_check(vm_t vm, value_t callable) {
  if (!callable || value_is_shadow(callable))
    return create_void_value(vm);

  ast_func_t af = (ast_func_t)value_get_data(callable);
  if (!af || !af->node)
    return create_void_value(vm);

  /* Run with shadow=true, no args needed for checking */
  return _ast_func_exec(vm, callable, 0, NULL, true);
}
