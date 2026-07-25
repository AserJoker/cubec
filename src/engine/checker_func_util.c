#include "engine/checker_func_util.h"
#include "engine/resolver.h"
#include "engine/scope.h"
#include "engine/semantic_type.h"
#include "engine/flow_state.h"
#include "engine/diagnostic.h"
#include "engine/type_hash.h"
#include "engine/checker_check_stmt.h"
#include "core/allocator.h"
#include "core/vec.h"
#include "core/type.h"
#include "cubec/function_argument.h"
#include "cubec/function_capture.h"
#include "cubec/generic_param.h"
#include "cubec/literal_identifier.h"
#include <string.h>

/* Forward declaration — defined in checker_check_stmt.c */
flow_state_t _check_statement(context_t ctx, node_t stmt,
                               semantic_type_t return_type);

/* Forward declaration — defined in checker_check_stmt.c */
const char *_checker_ident_str(node_t node);

/* ===== func_check_info constructors ===== */

void func_check_info_from_statement(func_check_info_t *info,
                                     cubec_statement_function_t node) {
  info->arguments = node->arguments;
  info->return_type = node->return_type;
  info->body = node->body;
  info->captures = node->captures;
  info->generic_params = node->generic_params;
  info->is_c_variadic = node->is_c_variadic;
  info->name = node->name;
  info->location = node->super.location;
  info->is_comptime = node->is_comptime;
  info->ast_node = (node_t)node;
  info->is_export = node->is_export;
  info->is_inline = node->is_inline;
  info->is_extern = node->is_extern;
  info->is_builtin = node->is_builtin;
  info->decorators = node->decorators;
}

void func_check_info_from_expression(func_check_info_t *info,
                                      cubec_expression_function_t node) {
  info->arguments = node->arguments;
  info->return_type = node->return_type;
  info->body = node->body;
  info->captures = node->captures;
  info->generic_params = node->generic_params;
  info->is_c_variadic = node->is_c_variadic;
  info->name = node->name;
  info->location = node->super.super.location;
  info->is_comptime = false;
  info->ast_node = (node_t)node;
  info->is_export = false;
  info->is_inline = false;
  info->is_extern = false;
  info->is_builtin = false;
  info->decorators = NULL;
}

/* ===== unified parameter type resolution ===== */

vec_t _resolve_func_param_types(context_t ctx, const func_check_info_t *info) {
  vec_init_t vi = {.auto_dispose = false};
  vec_t param_types = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &vi);
  if (!info->arguments) return param_types;
  size_t count = vec_get_size(info->arguments);
  for (size_t i = 0; i < count; i++) {
    node_t arg = (node_t)vec_get(info->arguments, i);
    if (arg->kind != CUBEC_NODE_FUNCTION_ARGUMENT) continue;
    cubec_function_argument_t farg = (cubec_function_argument_t)arg;
    semantic_type_t pt = farg->type
        ? resolver_resolve_type(ctx, farg->type) : ctx->error_type;
    vec_push(param_types, pt);
  }
  return param_types;
}

/* ===== unified parameter symbol registration ===== */

void _register_func_params_from_info(context_t ctx,
                                      const func_check_info_t *info,
                                      vec_t param_types) {
  if (!info->arguments) return;
  size_t count = vec_get_size(info->arguments);
  for (size_t j = 0; j < count; j++) {
    node_t arg = (node_t)vec_get(info->arguments, j);
    if (arg->kind != CUBEC_NODE_FUNCTION_ARGUMENT) continue;
    cubec_function_argument_t farg = (cubec_function_argument_t)arg;
    const char *pname = _checker_ident_str(farg->identifier);
    if (!pname) continue;
    struct symbol *psym = symbol_create(ctx->allocator, pname,
                                        SYMBOL_VARIABLE, arg->location);
    psym->variable.type = (param_types && j < vec_get_size(param_types))
                            ? (semantic_type_t)vec_get(param_types, j)
                            : ctx->error_type;
    psym->variable.is_mutable = !semantic_type_is_const(psym->variable.type);
    psym->state = SYMBOL_EVALUATED;
    scope_push_symbol(ctx->current_scope, psym);
  }
}

/* ===== unified capture registration ===== */

void _register_func_captures(context_t ctx, const func_check_info_t *info,
                              scope_t enclosing_scope) {
  if (!info->captures) return;
  size_t cc = vec_get_size(info->captures);
  for (size_t i = 0; i < cc; i++) {
    node_t cap_node = (node_t)vec_get(info->captures, i);
    if (cap_node->kind != CUBEC_NODE_FUNCTION_CAPTURE) continue;
    cubec_function_capture_t cap = (cubec_function_capture_t)cap_node;
    const char *cap_name = _checker_ident_str(cap->identifier);
    if (!cap_name) continue;
    struct symbol *outer_sym = scope_lookup(enclosing_scope, cap_name);
    if (outer_sym) {
      /* TDZ check: cannot capture a variable before initialization */
      if (outer_sym->state == SYMBOL_TDZ &&
          (!ctx->current_flow || flow_state_is_tdz(ctx->current_flow, cap_name))) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                             cap_node->location,
                             "cannot capture variable '%s' before initialization",
                             cap_name);
        ctx->error_count++;
      }
      struct symbol *cap_sym = symbol_create(ctx->allocator, cap_name,
                                              outer_sym->kind, cap_node->location);
      cap_sym->variable = outer_sym->variable;
      cap_sym->state = SYMBOL_EVALUATED;
      scope_push_symbol(ctx->current_scope, cap_sym);
    }
  }
}

/* ===== unified function body check + return exhaustiveness ===== */

void _check_func_body_and_returns(context_t ctx,
                                    const func_check_info_t *info,
                                    semantic_type_t return_type,
                                    vec_t param_types,
                                    scope_t scope_root) {
  if (!info->body) return;

  scope_t saved = ctx->current_scope;
  ctx->current_scope = scope_create(ctx->allocator, scope_root,
                                     SCOPE_FUNCTION, info->location);
  vec_push(ctx->all_scopes, ctx->current_scope);

  /* Register parameters */
  _register_func_params_from_info(ctx, info, param_types);

  /* Register captures (with TDZ check) */
  _register_func_captures(ctx, info, saved);

  /* Check function body */
  ctx->loop_depth = 0;
  flow_state_t fs = _check_statement(ctx, info->body, return_type);

  /* Return exhaustiveness check */
  if (return_type && return_type->impl->kind != TYPE_VOID &&
      !flow_state_is_all_returned(fs)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         info->location,
                         "non-void function must return a value on all paths");
    ctx->error_count++;
  }

  flow_state_dispose(fs, ctx->allocator);
  ctx->current_flow = NULL;
  ctx->current_scope = saved;
}

/* ===== unified generic params registration ===== */

void context_register_generic_params(context_t ctx, vec_t generic_params) {
  if (!generic_params) return;
  size_t count = vec_get_size(generic_params);
  bool seen_rest = false;
  for (size_t i = 0; i < count; i++) {
    node_t gp_node = (node_t)vec_get(generic_params, i);
    if (!gp_node || gp_node->kind != CUBEC_NODE_GENERIC_PARAM) continue;
    cubec_generic_param_t gp = (cubec_generic_param_t)gp_node;
    const char *gp_name = _checker_ident_str(gp->name);
    if (!gp_name) continue;

    /* Validate rest parameter rules */
    if (gp->is_rest) {
      if (seen_rest) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                            gp->super.location,
                            "only one rest parameter allowed");
        ctx->error_count++;
        continue;
      }
      if (i != count - 1) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                            gp->super.location,
                            "rest parameter must be last in generic parameter list");
        ctx->error_count++;
        continue;
      }
      seen_rest = true;
    }

    struct symbol *sym = symbol_create(ctx->allocator, gp_name,
                                       SYMBOL_GENERIC_PARAM, gp->super.location);
    sym->generic_param.is_rest = gp->is_rest;
    if (gp->constraints) {
      size_t ccount = vec_get_size(gp->constraints);
      vec_t cvec = allocator_create(ctx->allocator, &g_vec_type,
                                    &(vec_init_t){.auto_dispose = false});
      for (size_t j = 0; j < ccount; j++) {
        node_t cnode = (node_t)vec_get(gp->constraints, j);
        semantic_type_t ct = resolver_resolve_type(ctx, cnode);
        vec_push(cvec, ct);
      }
      sym->generic_param.constraints = cvec;
    }
    if (gp->value_type)
      sym->generic_param.value_type = resolver_resolve_type(ctx, gp->value_type);
    sym->state = SYMBOL_EVALUATED;
    scope_push_symbol(ctx->current_scope, sym);
  }
}

/* ===== unified function processing ===== */

semantic_type_t _process_function(context_t ctx, func_check_info_t *info,
                                   func_context_t *fctx) {
  /* 1. Register generic params */
  scope_t saved = ctx->current_scope;
  if (info->generic_params) {
    if (fctx->use_child_scope) {
      ctx->current_scope = scope_create(ctx->allocator, saved,
          SCOPE_BLOCK, info->location);
      vec_push(ctx->all_scopes, ctx->current_scope);
    }
    context_register_generic_params(ctx, info->generic_params);
  }

  /* 2. Resolve return type and parameter types */
  semantic_type_t ret_type = info->return_type
      ? resolver_resolve_type(ctx, info->return_type) : ctx->builtin_void;
  if (!ret_type) ret_type = ctx->builtin_void;
  vec_t param_types = _resolve_func_param_types(ctx, info);

  if (info->generic_params && fctx->use_child_scope)
    ctx->current_scope = saved;

  /* 3. Create function type */
  semantic_type_t ftype = semantic_type_create_function(
      ctx->allocator, ret_type, param_types, info->is_c_variadic);
  type_hash_ensure(ftype);
  vec_push(ctx->all_types, ftype);

  /* 4. Symbol binding */
  const char *name = info->name ? _checker_ident_str(info->name) : NULL;
  struct symbol *sym = fctx->pre_existing_sym;
  enum symbol_state sym_state = fctx->symbol_state;
  if (!sym && name && fctx->symbol_scope) {
    sym = symbol_create(ctx->allocator, name, SYMBOL_FUNCTION, info->location);
    sym->function.type = ftype;
    sym->function.is_comptime = info->is_comptime;
    sym->function.ast_node = info->ast_node;
    sym->function.generic_params = info->generic_params;
    sym->state = sym_state;
    if (fctx->is_method)
      vec_push(fctx->host_type->instance_methods, sym);
    else
      scope_push_symbol(fctx->symbol_scope, sym);
  } else if (sym) {
    sym->function.type = ftype;
    sym->function.is_comptime = info->is_comptime;
    sym->function.ast_node = info->ast_node;
    sym->function.generic_params = info->generic_params;
    sym->state = sym_state;
  }

  /* 5. Body checking */
  if (info->body && !fctx->defer_body) {
    _check_func_body_and_returns(ctx, info, ret_type, param_types, saved);
  }

  return ftype;
}
