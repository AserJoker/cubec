#include "engine/context.h"
#include "engine/checker_check_stmt.h"
#include "engine/checker_check_expr.h"
#include "engine/checker_func_util.h"
#include "engine/checker_type_util.h"
#include "engine/checker_collect.h"
#include "engine/checker_evaluate.h"
#include "engine/comptime_eval.h"
#include "engine/resolver.h"
#include "engine/type_hash.h"
#include "engine/type_layout.h"
#include "cubec/program.h"
#include "cubec/literal_identifier.h"
#include "cubec/statement_block.h"
#include "cubec/function_capture.h"
#include "cubec/function_argument.h"
#include "cubec/statement_expression.h"
#include "cubec/statement_return.h"
#include "cubec/generic_param.h"
#include "cubec/statement_struct.h"
#include "cubec/statement_union.h"
#include "cubec/statement_cunion.h"
#include "cubec/statement_enum.h"
#include "cubec/struct_field.h"
#include "cubec/union_field.h"
#include "cubec/enum_item.h"
#include "cubec/literal_numeric.h"
#include "cubec/statement_if.h"
#include "cubec/statement_while.h"
#include "cubec/statement_do_while.h"
#include "cubec/statement_for.h"
#include "cubec/statement_foreach.h"
#include "cubec/statement_break.h"
#include "cubec/statement_continue.h"
#include "cubec/statement_defer.h"
#include "cubec/statement_switch.h"
#include "cubec/statement_empty.h"
#include "cubec/statement_declaration.h"
#include "cubec/statement_comptime.h"
#include "cubec/statement_interface.h"
#include "cubec/statement_import.h"
#include "cubec/statement_test.h"
#include "cubec/switch_match.h"
#include "cubec/declaration_variable.h"
#include "cubec/expression_assignment.h"
#include <string.h>

/* ===== Pass 3: Statement Checking ===== */

flow_state_t _check_statement(context_t ctx, node_t stmt,
                               semantic_type_t return_type);

/* --- block --- */

static flow_state_t _check_stmt_block(context_t ctx,
                                       cubec_statement_block_t block,
                                       semantic_type_t return_type) {
  if (!block || !block->statements) {
    /* Inherit TDZ from parent flow context */
    if (ctx->current_flow)
      return flow_state_clone(ctx->allocator, ctx->current_flow);
    return flow_state_alive(ctx->allocator);
  }
  scope_t saved = ctx->current_scope;
  ctx->current_scope = scope_create(ctx->allocator, ctx->current_scope,
                                     SCOPE_BLOCK, block->super.location);
  vec_push(ctx->all_scopes, ctx->current_scope);
  size_t count = vec_get_size(block->statements);
  /* Inherit TDZ from parent flow context */
  flow_state_t fs;
  if (ctx->current_flow)
    fs = flow_state_clone(ctx->allocator, ctx->current_flow);
  else
    fs = flow_state_alive(ctx->allocator);
  bool unreachable_warned = false;
  for (size_t i = 0; i < count; i++) {
    node_t s = (node_t)vec_get(block->statements, i);
    /* Unreachable code detection */
    if (flow_state_is_unreachable(fs)) {
      if (!unreachable_warned) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_WARNING,
                             s->location, "unreachable code");
        unreachable_warned = true;
      }
      /* Skip checking unreachable statements — they can't affect flow */
      continue;
    }
    /* Set current_flow so expression checker can access TDZ info.
     * Save and restore around each statement to handle nesting. */
    flow_state_t saved_flow = ctx->current_flow;
    ctx->current_flow = fs;
    flow_state_t new_fs = _check_statement(ctx, s, return_type);
    ctx->current_flow = saved_flow;
    flow_state_dispose(fs, ctx->allocator);
    fs = new_fs;
  }
  ctx->current_scope = saved;
  return fs;
}

/* --- if --- */

static flow_state_t _check_stmt_if(context_t ctx, node_t stmt,
                                    semantic_type_t return_type) {
  cubec_statement_if_t sif = (cubec_statement_if_t)stmt;
  semantic_type_t ct = _check_expression(ctx, sif->condition);
  if (!_is_bool_type(ct)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         stmt->location,
                         "if condition must be bool");
    ctx->error_count++;
  }
  /* Save the flow state before branching so both branches inherit it */
  flow_state_t pre_flow = ctx->current_flow
      ? flow_state_clone(ctx->allocator, ctx->current_flow)
      : flow_state_alive(ctx->allocator);

  flow_state_t then_fs = _check_statement(ctx, sif->then_branch, return_type);
  flow_state_t else_fs;
  if (sif->else_branch) {
    else_fs = _check_statement(ctx, sif->else_branch, return_type);
  } else {
    /* No else branch: the pre-if flow state continues on the false path */
    else_fs = pre_flow;
    pre_flow = NULL; /* prevent double-free */
  }
  flow_state_t merged = flow_state_merge(ctx->allocator, then_fs, else_fs);
  flow_state_dispose(then_fs, ctx->allocator);
  flow_state_dispose(else_fs, ctx->allocator);
  if (pre_flow) flow_state_dispose(pre_flow, ctx->allocator);
  return merged;
}

/* --- while --- */

static flow_state_t _check_stmt_while(context_t ctx, node_t stmt,
                                       semantic_type_t return_type) {
  cubec_statement_while_t sw = (cubec_statement_while_t)stmt;
  semantic_type_t ct = _check_expression(ctx, sw->condition);
  if (!_is_bool_type(ct)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         stmt->location,
                         "while condition must be bool");
    ctx->error_count++;
  }
  /* Save pre-loop flow state (body may not execute) */
  flow_state_t pre_flow = ctx->current_flow
      ? flow_state_clone(ctx->allocator, ctx->current_flow)
      : flow_state_alive(ctx->allocator);
  ctx->loop_depth++;
  flow_state_t body_fs = _check_statement(ctx, sw->body, return_type);
  ctx->loop_depth--;
  /* Loop body may not execute (condition could be false initially),
   * so the path after the loop inherits the pre-loop flow state.
   * TDZ assignments in loop body don't propagate outward. */
  flow_state_dispose(body_fs, ctx->allocator);
  return pre_flow;
}

/* --- do-while --- */

static flow_state_t _check_stmt_do_while(context_t ctx, node_t stmt,
                                          semantic_type_t return_type) {
  cubec_statement_do_while_t dw = (cubec_statement_do_while_t)stmt;
  /* Save pre-loop flow state (loop may not continue after first iteration) */
  flow_state_t pre_flow = ctx->current_flow
      ? flow_state_clone(ctx->allocator, ctx->current_flow)
      : flow_state_alive(ctx->allocator);
  ctx->loop_depth++;
  flow_state_t body_fs = _check_statement(ctx, dw->body, return_type);
  ctx->loop_depth--;
  semantic_type_t ct = _check_expression(ctx, dw->condition);
  if (!_is_bool_type(ct)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         stmt->location,
                         "do-while condition must be bool");
    ctx->error_count++;
  }
  /* do-while body executes at least once, but loop may not continue;
   * path after the loop inherits the pre-loop flow state. */
  flow_state_dispose(body_fs, ctx->allocator);
  return pre_flow;
}

/* --- for --- */

static flow_state_t _check_stmt_for(context_t ctx, node_t stmt,
                                     semantic_type_t return_type) {
  cubec_statement_for_t sf = (cubec_statement_for_t)stmt;
  scope_t saved = ctx->current_scope;
  ctx->current_scope = scope_create(ctx->allocator, ctx->current_scope,
                                     SCOPE_FOR, stmt->location);
  vec_push(ctx->all_scopes, ctx->current_scope);
  if (sf->init) {
    flow_state_t init_fs = _check_statement(ctx, sf->init, return_type);
    flow_state_dispose(init_fs, ctx->allocator);
  }
  if (sf->condition) {
    semantic_type_t ct = _check_expression(ctx, sf->condition);
    if (!_is_bool_type(ct)) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           stmt->location,
                           "for condition must be bool");
      ctx->error_count++;
    }
  }
  if (sf->increment) _check_expression(ctx, sf->increment);
  /* Save pre-loop flow state (body may not execute) */
  flow_state_t pre_flow = ctx->current_flow
      ? flow_state_clone(ctx->allocator, ctx->current_flow)
      : flow_state_alive(ctx->allocator);
  ctx->loop_depth++;
  flow_state_t body_fs = _check_statement(ctx, sf->body, return_type);
  ctx->loop_depth--;
  ctx->current_scope = saved;
  /* Loop body may not execute */
  flow_state_dispose(body_fs, ctx->allocator);
  return pre_flow;
}

/* --- foreach --- */

static flow_state_t _check_stmt_foreach(context_t ctx, node_t stmt,
                                         semantic_type_t return_type) {
  cubec_statement_foreach_t sfe = (cubec_statement_foreach_t)stmt;
  semantic_type_t iter_type = _check_expression(ctx, sfe->iterator);
  scope_t saved = ctx->current_scope;
  ctx->current_scope = scope_create(ctx->allocator, ctx->current_scope,
                                     SCOPE_FOREACH, stmt->location);
  vec_push(ctx->all_scopes, ctx->current_scope);
  const char *vname = _checker_ident_str(sfe->variable);
  if (vname) {
    struct symbol *vsym = symbol_create(ctx->allocator, vname,
                                         SYMBOL_VARIABLE, stmt->location);
    /* Derive element type from iterator or use explicit type */
    if (sfe->var_type) {
      vsym->variable.type = resolver_resolve_type(ctx, sfe->var_type);
      if (!vsym->variable.type) vsym->variable.type = ctx->error_type;
    } else if (iter_type->impl->kind == TYPE_SLICE)
      vsym->variable.type = iter_type->impl->slice.element;
    else if (iter_type->impl->kind == TYPE_ARRAY)
      vsym->variable.type = iter_type->impl->array.element;
    else if (iter_type->impl->kind == TYPE_STRING)
      vsym->variable.type = ctx->builtin_char;
    else {
      /* Iterator protocol: look for next() in instance_methods.
       * The element type is the return type's "value" field type. */
      semantic_type_t elem_type = NULL;
      if (iter_type->instance_methods) {
        size_t mc = vec_get_size(iter_type->instance_methods);
        for (size_t i = 0; i < mc; i++) {
          struct symbol *s = (struct symbol *)vec_get(iter_type->instance_methods, i);
          if (s && s->name && strcmp(s->name, "next") == 0 &&
              s->kind == SYMBOL_FUNCTION && s->function.type) {
            /* next() return type should be a struct with "value" field */
            semantic_type_t next_ret =
                s->function.type->impl->function.return_type;
            if (next_ret) {
              vec_t fields = NULL;
              if (next_ret->impl->kind == TYPE_STRUCT)
                fields = next_ret->impl->struct_type.fields;
              else if (next_ret->impl->kind == TYPE_GENERIC_INSTANCE)
                fields = next_ret->impl->generic_instance.fields;
              if (fields) {
                size_t fc = vec_get_size(fields);
                for (size_t j = 0; j < fc; j++) {
                  struct symbol *fs = (struct symbol *)vec_get(fields, j);
                  if (fs && fs->name && strcmp(fs->name, "value") == 0 &&
                      fs->kind == SYMBOL_FIELD) {
                    elem_type = fs->field.type;
                    break;
                  }
                }
              }
            }
            break;
          }
        }
      }
      vsym->variable.type = elem_type ? elem_type : ctx->error_type;
    }
    vsym->variable.is_mutable = !semantic_type_is_const(vsym->variable.type);
    vsym->state = SYMBOL_EVALUATED;
    scope_push_symbol(ctx->current_scope, vsym);
  }
  /* Save pre-loop flow state (body may not execute) */
  flow_state_t pre_flow = ctx->current_flow
      ? flow_state_clone(ctx->allocator, ctx->current_flow)
      : flow_state_alive(ctx->allocator);
  ctx->loop_depth++;
  flow_state_t body_fs = _check_statement(ctx, sfe->body, return_type);
  ctx->loop_depth--;
  ctx->current_scope = saved;
  /* Loop body may not execute */
  flow_state_dispose(body_fs, ctx->allocator);
  return pre_flow;
}

/* --- switch --- */

static flow_state_t _check_stmt_switch(context_t ctx, node_t stmt,
                                        semantic_type_t return_type) {
  cubec_statement_switch_t ss = (cubec_statement_switch_t)stmt;
  _check_expression(ctx, ss->condition);
  flow_state_t merged = flow_state_alive(ctx->allocator);
  bool has_else = false;
  if (ss->matches) {
    size_t mcount = vec_get_size(ss->matches);
    for (size_t i = 0; i < mcount; i++) {
      node_t match = (node_t)vec_get(ss->matches, i);
      if (match->kind == CUBEC_NODE_SWITCH_MATCH) {
        cubec_switch_match_t sm = (cubec_switch_match_t)match;
        if (sm->is_else) has_else = true;
        if (sm->values) {
          size_t vcount = vec_get_size(sm->values);
          for (size_t j = 0; j < vcount; j++) {
            _check_expression(ctx, (node_t)vec_get(sm->values, j));
          }
        }
        flow_state_t arm_fs = _check_statement(ctx, sm->body, return_type);
        flow_state_t new_merged = flow_state_merge(ctx->allocator, merged, arm_fs);
        flow_state_dispose(merged, ctx->allocator);
        flow_state_dispose(arm_fs, ctx->allocator);
        merged = new_merged;
      }
    }
  }
  /* If no `_` (else) branch, implicit path where no match case hit.
   * This path inherits the pre-switch flow state (with TDZ info). */
  if (!has_else) {
    flow_state_t else_fs = ctx->current_flow
        ? flow_state_clone(ctx->allocator, ctx->current_flow)
        : flow_state_alive(ctx->allocator);
    flow_state_t new_merged = flow_state_merge(ctx->allocator, merged, else_fs);
    flow_state_dispose(merged, ctx->allocator);
    flow_state_dispose(else_fs, ctx->allocator);
    merged = new_merged;
  }
  return merged;
}

/* --- declaration --- */

static flow_state_t _check_stmt_declaration(context_t ctx, node_t stmt) {
  cubec_statement_declaration_t sdecl =
      (cubec_statement_declaration_t)stmt;
  cubec_declaration_variable_t vdecl =
      (cubec_declaration_variable_t)sdecl->declarator;
  if (!vdecl) return flow_state_alive(ctx->allocator);

  const char *vname = _checker_ident_str(vdecl->identifier);
  if (!vname) return flow_state_alive(ctx->allocator);

  /* Check if this is an undefined initializer */
  bool is_undefined_init = vdecl->expression &&
      vdecl->expression->kind == CUBEC_NODE_LITERAL_UNDEFINED;

  /* Non-extern/non-builtin declarations require an initializer */
  if (!sdecl->is_extern && !sdecl->is_builtin && !vdecl->expression) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         stmt->location,
                         "variable '%s' requires an initializer", vname);
    ctx->error_count++;
  }

  /* Local comptime variables require an initializer and cannot use undefined.
     Value must be known at compile time. */
  if (sdecl->is_comptime) {
    if (is_undefined_init) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           stmt->location,
                           "comptime variable '%s' cannot be initialized with 'undefined' — value must be known at compile time",
                           vname);
      ctx->error_count++;
    }
    if (!vdecl->expression) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           stmt->location,
                           "comptime variable '%s' requires an initializer — value must be known at compile time",
                           vname);
      ctx->error_count++;
    }
  }

  /* 'using' not allowed at module scope */
  if (sdecl->is_using && scope_get_kind(ctx->current_scope) == SCOPE_GLOBAL) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         stmt->location,
                         "'using' declaration not allowed at module scope");
    ctx->error_count++;
  }

  /* 'using' cannot be initialized with undefined (defer would always skip __dispose__) */
  if (sdecl->is_using && is_undefined_init) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         stmt->location,
                         "'using' variable cannot be initialized with undefined");
    ctx->error_count++;
  }

  semantic_type_t var_type = NULL;

  if (is_undefined_init) {
    /* undefined initializer: type must come from annotation, variable is TDZ */
    if (vdecl->type) {
      var_type = resolver_resolve_type(ctx, vdecl->type);
    }
    if (!var_type) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           stmt->location,
                           "cannot infer type for variable '%s' with undefined initializer",
                           vname);
      ctx->error_count++;
      var_type = ctx->error_type;
    }
  } else {
    /* Normal initializer or no initializer (extern/builtin) */
    if (vdecl->type)
      var_type = resolver_resolve_type(ctx, vdecl->type);

    if (vdecl->expression) {
      semantic_type_t init_type = _check_expression(ctx, vdecl->expression);

      if (var_type) {
        /* Explicit type annotation: check that init is compatible */
        if (init_type && init_type->impl->kind != TYPE_ERROR &&
            var_type->impl->kind != TYPE_ERROR &&
            !semantic_type_can_implicit_convert(init_type, var_type)) {
          diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                               stmt->location,
                               "cannot assign '%s' to '%s'",
                               init_type->name ? init_type->name : "<anonymous>",
                               var_type->name ? var_type->name : "<anonymous>");
          ctx->error_count++;
        }
      } else {
        /* No type annotation: infer from initializer */
        var_type = init_type;
      }
    }

    if (!var_type) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           stmt->location,
                           "cannot infer type for variable '%s'", vname);
      ctx->error_count++;
      var_type = ctx->error_type;
    }
  }

  /* Local comptime variables are implicitly const */
  if (sdecl->is_comptime && var_type && var_type->impl->kind != TYPE_ERROR &&
      !semantic_type_is_const(var_type)) {
    semantic_type_t const_type = semantic_type_create_qualifier(
        ctx->allocator, var_type, true, false);
    type_hash_ensure(const_type);
    vec_push(ctx->all_types, const_type);
    var_type = const_type;
  }

  struct symbol *vsym = symbol_create(ctx->allocator, vname,
                                       SYMBOL_VARIABLE, stmt->location);
  vsym->variable.type = var_type;
  vsym->variable.is_comptime = sdecl->is_comptime;
  vsym->variable.is_mutable = !semantic_type_is_const(var_type);
  vsym->variable.is_using = sdecl->is_using;
  /* undefined initializer → TDZ; otherwise → EVALUATED */
  vsym->state = is_undefined_init ? SYMBOL_TDZ : SYMBOL_EVALUATED;
  scope_push_symbol(ctx->current_scope, vsym);

  /* 'using' requires the type to implement __dispose__ */
  if (sdecl->is_using && var_type && var_type->impl->kind != TYPE_ERROR) {
    struct symbol *dispose_sym = NULL;
    if (var_type->instance_methods) {
      size_t mc = vec_get_size(var_type->instance_methods);
      for (size_t i = 0; i < mc; i++) {
        struct symbol *m = (struct symbol *)vec_get(var_type->instance_methods, i);
        if (m && m->name && strcmp(m->name, "__dispose__") == 0) {
          dispose_sym = m;
          break;
        }
      }
    }
    if (!dispose_sym) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           stmt->location,
                           "type '%s' must implement '__dispose__' for 'using' declaration",
                           var_type->name ? var_type->name : "<anonymous>");
      ctx->error_count++;
    } else if (dispose_sym->function.type &&
               dispose_sym->function.type->impl->kind == TYPE_FUNCTION) {
      if (dispose_sym->function.type->impl->function.return_type &&
          dispose_sym->function.type->impl->function.return_type->impl->kind != TYPE_VOID) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                             stmt->location,
                             "'__dispose__' must return void");
        ctx->error_count++;
      }
    }
  }

  /* Local comptime variable: evaluate initializer at compile time and bind to
     comptime env so it's available for comptime if/foreach conditions. */
  if (sdecl->is_comptime && vdecl->expression && ctx->comptime_eval) {
    comptime_value_t val =
        comptime_eval_expr(ctx->comptime_eval, ctx, vdecl->expression);
    if (val && val->kind != COMPTIME_VALUE_ERROR) {
      comptime_env_bind_value(
          ctx->comptime_eval->current_env
              ? ctx->comptime_eval->current_env
              : ctx->comptime_eval->global_env,
          ctx->comptime_eval->valloc,
          vname, comptime_value_clone(ctx->allocator, val));
    }
  }

  /* Return flow state with TDZ tracking */
  flow_state_t fs = flow_state_alive(ctx->allocator);
  if (is_undefined_init && vname) {
    flow_state_add_tdz(fs, vname);
  }
  return fs;
}

/* --- return --- */

static flow_state_t _check_stmt_return(context_t ctx, node_t stmt,
                                        semantic_type_t return_type) {
  cubec_statement_return_t ret = (cubec_statement_return_t)stmt;
  if (ret->expression) {
    semantic_type_t et = _check_expression(ctx, ret->expression);
    if (return_type && return_type->impl->kind != TYPE_ERROR &&
        et->impl->kind != TYPE_ERROR) {
      if (!semantic_type_can_implicit_convert(et, return_type)) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                             stmt->location,
                             "cannot return '%s' from function returning '%s'",
                             et->name ? et->name : "<anonymous>",
                             return_type->name ? return_type->name
                                               : "<anonymous>");
        ctx->error_count++;
      }
    }
  } else {
    if (return_type && return_type->impl->kind != TYPE_VOID) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           stmt->location,
                           "missing return value in function returning '%s'",
                           return_type->name ? return_type->name
                                             : "<anonymous>");
      ctx->error_count++;
    }
  }
  flow_state_t fs = flow_state_create(ctx->allocator);
  flow_state_mark_returned(fs);
  return fs;
}

/* --- defer --- */

static flow_state_t _check_stmt_defer(context_t ctx, node_t stmt,
                                       semantic_type_t return_type) {
  cubec_statement_defer_t sd = (cubec_statement_defer_t)stmt;

  /* TDZ check: cannot capture a variable before initialization */
  if (sd->captures) {
    size_t cc = vec_get_size(sd->captures);
    for (size_t i = 0; i < cc; i++) {
      node_t cap_node = (node_t)vec_get(sd->captures, i);
      if (cap_node->kind != CUBEC_NODE_FUNCTION_CAPTURE) continue;
      cubec_function_capture_t cap = (cubec_function_capture_t)cap_node;
      const char *cap_name = _checker_ident_str(cap->identifier);
      if (!cap_name) continue;
      struct symbol *outer_sym = scope_lookup(ctx->current_scope, cap_name);
      if (outer_sym) {
        if (outer_sym->state == SYMBOL_TDZ &&
            (!ctx->current_flow || flow_state_is_tdz(ctx->current_flow, cap_name))) {
          diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                               cap_node->location,
                               "cannot capture variable '%s' before initialization",
                               cap_name);
          ctx->error_count++;
        }
      }
    }
  }

  /* Check the defer body, but defer doesn't affect control flow */
  flow_state_t body_fs = _check_statement(ctx, sd->body, return_type);
  flow_state_dispose(body_fs, ctx->allocator);
  return flow_state_alive(ctx->allocator);
}

/* --- comptime if --- */

static flow_state_t _check_stmt_comptime_if(context_t ctx, node_t stmt,
                                             semantic_type_t return_type) {
  cubec_statement_comptime_if_t ci =
      (cubec_statement_comptime_if_t)stmt;
  if (ci->condition) _check_expression(ctx, ci->condition);

  /* Evaluate condition at comptime to determine taken branch */
  if (ctx->comptime_eval && ci->condition) {
    comptime_value_t cond =
        comptime_eval_expr(ctx->comptime_eval, ctx, ci->condition);
    if (!cond || !cond || cond->kind == COMPTIME_VALUE_ERROR ||
        cond->kind == COMPTIME_VALUE_FATAL) {
      ctx->error_count++;
      return flow_state_alive(ctx->allocator);
    }
    if (cond->kind != COMPTIME_VALUE_BOOL) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, stmt->location,
                           "comptime if condition must be a compile-time bool");
      ctx->error_count++;
      return flow_state_alive(ctx->allocator);
    }

    /* Only check the taken branch */
    bool condition_true = comptime_value_is_truthy(cond);
    node_t taken = condition_true ? ci->then_branch : ci->else_branch;
    if (taken) {
      return _check_statement(ctx, taken, return_type);
    }
    return flow_state_alive(ctx->allocator);
  }

  /* Fallback: no comptime eval, check both branches */
  flow_state_t pre_flow = ctx->current_flow
      ? flow_state_clone(ctx->allocator, ctx->current_flow)
      : flow_state_alive(ctx->allocator);
  flow_state_t then_fs = _check_statement(ctx, ci->then_branch, return_type);
  flow_state_t else_fs;
  if (ci->else_branch) {
    else_fs = _check_statement(ctx, ci->else_branch, return_type);
  } else {
    else_fs = pre_flow;
    pre_flow = NULL;
  }
  flow_state_t merged = flow_state_merge(ctx->allocator, then_fs, else_fs);
  flow_state_dispose(then_fs, ctx->allocator);
  flow_state_dispose(else_fs, ctx->allocator);
  if (pre_flow) flow_state_dispose(pre_flow, ctx->allocator);
  return merged;
}

/* --- comptime foreach --- */

static flow_state_t _check_stmt_comptime_foreach(context_t ctx, node_t stmt,
                                                   semantic_type_t return_type) {
  cubec_statement_comptime_foreach_t cf =
      (cubec_statement_comptime_foreach_t)stmt;
  semantic_type_t iter_type = _check_expression(ctx, cf->iterator);

  scope_t saved = ctx->current_scope;
  ctx->current_scope = scope_create(ctx->allocator, ctx->current_scope,
                                     SCOPE_COMPTIME, stmt->location);
  vec_push(ctx->all_scopes, ctx->current_scope);

  const char *vname = _checker_ident_str(cf->variable);
  if (vname) {
    struct symbol *vsym = symbol_create(ctx->allocator, vname,
                                         SYMBOL_VARIABLE, stmt->location);
    /* Derive element type from iterator or use explicit type */
    if (cf->var_type) {
      vsym->variable.type = resolver_resolve_type(ctx, cf->var_type);
      if (!vsym->variable.type) vsym->variable.type = ctx->error_type;
    } else if (iter_type->impl->kind == TYPE_SLICE)
      vsym->variable.type = iter_type->impl->slice.element;
    else if (iter_type->impl->kind == TYPE_ARRAY)
      vsym->variable.type = iter_type->impl->array.element;
    else if (iter_type->impl->kind == TYPE_STRING)
      vsym->variable.type = ctx->builtin_char;
    else {
      /* Iterator protocol: look for next() in instance_methods */
      semantic_type_t elem_type = NULL;
      if (iter_type->instance_methods) {
        size_t mc = vec_get_size(iter_type->instance_methods);
        for (size_t i = 0; i < mc; i++) {
          struct symbol *s = (struct symbol *)vec_get(iter_type->instance_methods, i);
          if (s && s->name && strcmp(s->name, "next") == 0 &&
              s->kind == SYMBOL_FUNCTION && s->function.type) {
            semantic_type_t next_ret =
                s->function.type->impl->function.return_type;
            if (next_ret) {
              vec_t fields = NULL;
              if (next_ret->impl->kind == TYPE_STRUCT)
                fields = next_ret->impl->struct_type.fields;
              else if (next_ret->impl->kind == TYPE_GENERIC_INSTANCE)
                fields = next_ret->impl->generic_instance.fields;
              if (fields) {
                size_t fc = vec_get_size(fields);
                for (size_t j = 0; j < fc; j++) {
                  struct symbol *fs = (struct symbol *)vec_get(fields, j);
                  if (fs && fs->name && strcmp(fs->name, "value") == 0 &&
                      fs->kind == SYMBOL_FIELD) {
                    elem_type = fs->field.type;
                    break;
                  }
                }
              }
            }
            break;
          }
        }
      }
      vsym->variable.type = elem_type ? elem_type : ctx->error_type;
    }
    vsym->variable.is_mutable = !semantic_type_is_const(vsym->variable.type);
    vsym->state = SYMBOL_EVALUATED;
    scope_push_symbol(ctx->current_scope, vsym);
  }

  /* Save pre-loop flow state (body may not execute) */
  flow_state_t pre_flow = ctx->current_flow
      ? flow_state_clone(ctx->allocator, ctx->current_flow)
      : flow_state_alive(ctx->allocator);
  ctx->loop_depth++;
  flow_state_t body_fs = cf->body
      ? _check_statement(ctx, cf->body, return_type)
      : flow_state_alive(ctx->allocator);
  ctx->loop_depth--;
  ctx->current_scope = saved;
  flow_state_dispose(body_fs, ctx->allocator);
  return pre_flow;
}

/* --- local type declaration checkers (forward declarations) --- */

static flow_state_t _check_stmt_local_function(context_t ctx,
                                                cubec_statement_function_t node);
static flow_state_t _check_stmt_local_struct(context_t ctx,
                                              cubec_statement_struct_t node);
static flow_state_t _check_stmt_local_union(context_t ctx,
                                             cubec_statement_union_t node);
static flow_state_t _check_stmt_local_cunion(context_t ctx,
                                              cubec_statement_cunion_t node);
static flow_state_t _check_stmt_local_enum(context_t ctx,
                                            cubec_statement_enum_t node);
static semantic_type_t _find_method_type(context_t ctx, semantic_type_t t,
                                         const char *mname);

/* --- statement dispatch --- */

static flow_state_t _check_stmt_invalid_declaration(context_t ctx, node_t stmt) {
  if (stmt->kind == CUBEC_NODE_STATEMENT_STRUCT ||
      stmt->kind == CUBEC_NODE_STATEMENT_ENUM ||
      stmt->kind == CUBEC_NODE_STATEMENT_UNION ||
      stmt->kind == CUBEC_NODE_STATEMENT_CUNION ||
      stmt->kind == CUBEC_NODE_STATEMENT_FUNCTION ||
      stmt->kind == CUBEC_NODE_STATEMENT_INTERFACE ||
      stmt->kind == CUBEC_NODE_STATEMENT_IMPORT ||
      stmt->kind == CUBEC_NODE_STATEMENT_EXPORT_FROM ||
      stmt->kind == CUBEC_NODE_STATEMENT_TEST) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, stmt->location,
                         "declaration not allowed in this scope");
    ctx->error_count++;
  }
  return flow_state_alive(ctx->allocator);
}

static flow_state_t _check_stmt_break_or_continue(context_t ctx, node_t stmt,
                                                   const char *keyword) {
  if (ctx->loop_depth <= 0) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, stmt->location,
                         "%s outside of loop", keyword);
    ctx->error_count++;
  }
  flow_state_t fs = flow_state_create(ctx->allocator);
  if (keyword[0] == 'b')
    flow_state_mark_broke(fs);
  else
    flow_state_mark_continued(fs);
  return fs;
}

flow_state_t _check_statement(context_t ctx, node_t stmt,
                              semantic_type_t return_type) {
  if (!stmt) return flow_state_alive(ctx->allocator);
  flow_state_t fs;
  switch (stmt->kind) {
  case CUBEC_NODE_STATEMENT_BLOCK:
    fs = _check_stmt_block(ctx, (cubec_statement_block_t)stmt, return_type);
    break;
  case CUBEC_NODE_STATEMENT_EXPRESSION: {
    node_t inner = ((cubec_statement_expression_t)stmt)->expression;
    semantic_type_t t = _check_expression(ctx, inner);
    if (t && t->impl->kind != TYPE_ERROR && t->impl->kind != TYPE_VOID) {
      /* Check if the expression is a wildcard assignment: _ = expr */
      bool is_discard = false;
      if (inner && inner->kind == CUBEC_NODE_EXPRESSION_ASSIGNMENT) {
        cubec_expression_assignment_t asgn = (cubec_expression_assignment_t)inner;
        if (asgn->left && asgn->left->kind == CUBEC_NODE_LITERAL_IDENTIFIER) {
          const char *lname = _checker_ident_str(asgn->left);
          if (lname && lname[0] == '_' && lname[1] == '\0')
            is_discard = true;
        }
      }
      if (!is_discard) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_WARNING, stmt->location,
                             "value of type '%s' is not used; use '_ = expr' to explicitly discard",
                             t->name ? t->name : "<anonymous>");
      }
    }
    /* Inherit TDZ info from current_flow (may have been modified by assignment) */
    if (ctx->current_flow) {
      fs = flow_state_clone(ctx->allocator, ctx->current_flow);
    } else {
      fs = flow_state_alive(ctx->allocator);
    }
    break;
  }
  case CUBEC_NODE_STATEMENT_RETURN:
    fs = _check_stmt_return(ctx, stmt, return_type);
    break;
  case CUBEC_NODE_STATEMENT_IF:
    fs = _check_stmt_if(ctx, stmt, return_type);
    break;
  case CUBEC_NODE_STATEMENT_WHILE:
    fs = _check_stmt_while(ctx, stmt, return_type);
    break;
  case CUBEC_NODE_STATEMENT_DO_WHILE:
    fs = _check_stmt_do_while(ctx, stmt, return_type);
    break;
  case CUBEC_NODE_STATEMENT_FOR:
    fs = _check_stmt_for(ctx, stmt, return_type);
    break;
  case CUBEC_NODE_STATEMENT_FOREACH:
    fs = _check_stmt_foreach(ctx, stmt, return_type);
    break;
  case CUBEC_NODE_STATEMENT_BREAK:
    fs = _check_stmt_break_or_continue(ctx, stmt, "break");
    break;
  case CUBEC_NODE_STATEMENT_CONTINUE:
    fs = _check_stmt_break_or_continue(ctx, stmt, "continue");
    break;
  case CUBEC_NODE_STATEMENT_DEFER:
    fs = _check_stmt_defer(ctx, stmt, return_type);
    break;
  case CUBEC_NODE_STATEMENT_SWITCH:
    fs = _check_stmt_switch(ctx, stmt, return_type);
    break;
  case CUBEC_NODE_STATEMENT_DECLARATION:
    fs = _check_stmt_declaration(ctx, stmt);
    break;
  case CUBEC_NODE_STATEMENT_FUNCTION:
    fs = _check_stmt_local_function(ctx, (cubec_statement_function_t)stmt);
    break;
  case CUBEC_NODE_STATEMENT_STRUCT:
    fs = _check_stmt_local_struct(ctx, (cubec_statement_struct_t)stmt);
    break;
  case CUBEC_NODE_STATEMENT_UNION:
    fs = _check_stmt_local_union(ctx, (cubec_statement_union_t)stmt);
    break;
  case CUBEC_NODE_STATEMENT_CUNION:
    fs = _check_stmt_local_cunion(ctx, (cubec_statement_cunion_t)stmt);
    break;
  case CUBEC_NODE_STATEMENT_ENUM:
    fs = _check_stmt_local_enum(ctx, (cubec_statement_enum_t)stmt);
    break;
  case CUBEC_NODE_STATEMENT_EMPTY:
    fs = flow_state_alive(ctx->allocator);
    break;
  case CUBEC_NODE_STATEMENT_ERROR:
  case CUBEC_NODE_ERROR:
    /* Parse error recovery placeholder — skip, diagnostic already recorded. */
    fs = flow_state_alive(ctx->allocator);
    break;
  case CUBEC_NODE_STATEMENT_COMPTIME_IF:
    fs = _check_stmt_comptime_if(ctx, stmt, return_type);
    break;
  case CUBEC_NODE_STATEMENT_COMPTIME_FOREACH:
    fs = _check_stmt_comptime_foreach(ctx, stmt, return_type);
    break;
  default:
    fs = _check_stmt_invalid_declaration(ctx, stmt);
    break;
  }
  return fs;
}

/* --- function body checker --- */

static flow_state_t _check_stmt_local_function(context_t ctx,
                                                cubec_statement_function_t node) {
  if (!node) return flow_state_alive(ctx->allocator);

  func_check_info_t info;
  func_check_info_from_statement(&info, node);

  semantic_type_t ftype = _process_function(ctx, &info, &(func_context_t){
      .symbol_scope = ctx->current_scope,
      .defer_body = false,
      .is_method = false,
      .host_type = NULL,
      .use_child_scope = false,
      .pre_existing_sym = NULL,
      .symbol_state = SYMBOL_EVALUATED
  });

  /* Bind local comptime function to comptime env so it can be called at
     compile time. Non-comptime local functions are not bound (they are not
     available for comptime execution at the local level). */
  if (info.is_comptime && info.body && !info.generic_params &&
      ftype && ctx->comptime_eval) {
    const char *fname = info.name ? _checker_ident_str(info.name) : NULL;
    if (fname) {
      vec_t param_names = NULL;
      if (info.arguments) {
        vec_init_t pvi = {.auto_dispose = false};
        param_names = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &pvi);
        size_t acount = vec_get_size(info.arguments);
        for (size_t i = 0; i < acount; i++) {
          node_t arg = (node_t)vec_get(info.arguments, i);
          if (arg->kind == CUBEC_NODE_FUNCTION_ARGUMENT) {
            cubec_function_argument_t farg = (cubec_function_argument_t)arg;
            const char *pname = _checker_ident_str(farg->identifier);
            if (pname) vec_push(param_names, (void *)pname);
          }
        }
      }
      comptime_value_t fn_val = comptime_value_create_function(
          ctx->allocator,
          ctx->comptime_eval->current_env
              ? ctx->comptime_eval->current_env
              : ctx->comptime_eval->global_env,
          info.body, param_names, ftype);
      comptime_env_bind_value(
          ctx->comptime_eval->current_env
              ? ctx->comptime_eval->current_env
              : ctx->comptime_eval->global_env,
          ctx->comptime_eval->valloc, fname, fn_val);
    }
  }

  return flow_state_alive(ctx->allocator);
}

/* --- local struct declaration checker --- */

static flow_state_t _check_stmt_local_struct(context_t ctx,
                                              cubec_statement_struct_t node) {
  if (!node) return flow_state_alive(ctx->allocator);
  const char *name = _checker_ident_str(node->name);
  if (!name) return flow_state_alive(ctx->allocator);

  /* Create type and register in current scope */
  semantic_type_t t = semantic_type_create_named(ctx->allocator, name, TYPE_STRUCT);
  vec_push(ctx->all_types, t);

  struct symbol *sym = symbol_create(ctx->allocator, name,
                                      SYMBOL_TYPE, node->super.location);
  sym->type.type = t;
  sym->state = SYMBOL_EVALUATED;
  scope_push_symbol(ctx->current_scope, sym);
  strmap_insert(ctx->type_name_table, name, t);

  /* Resolve fields and methods */
  _resolve_struct_fields(ctx, t, node->members);
  context_evaluate_struct_union_members(ctx, t, node->members, 0);

  /* Compute layout */
  type_layout_compute(t, 8);
  type_hash_ensure(t);

  /* Check method bodies using unified helper */
  if (node->members) {
    size_t mcount = vec_get_size(node->members);
    for (size_t i = 0; i < mcount; i++) {
      node_t m = (node_t)vec_get(node->members, i);
      if (m->kind != CUBEC_NODE_STATEMENT_FUNCTION) continue;
      cubec_statement_function_t mfn = (cubec_statement_function_t)m;
      if (!mfn->body) continue;

      const char *mname = _checker_ident_str(mfn->name);
      semantic_type_t mtype = _find_method_type(ctx, t, mname);
      if (!mtype) continue;

      func_check_info_t minfo;
      func_check_info_from_statement(&minfo, mfn);
      _check_func_body_and_returns(ctx, &minfo,
                                   mtype->impl->function.return_type,
                                   mtype->impl->function.params,
                                   ctx->current_scope);
    }
  }
  return flow_state_alive(ctx->allocator);
}

/* --- local union declaration checker --- */

static flow_state_t _check_stmt_local_union(context_t ctx,
                                             cubec_statement_union_t node) {
  if (!node) return flow_state_alive(ctx->allocator);
  const char *name = _checker_ident_str(node->name);
  if (!name) return flow_state_alive(ctx->allocator);

  semantic_type_t t = semantic_type_create_named(ctx->allocator, name, TYPE_UNION);
  vec_push(ctx->all_types, t);

  struct symbol *sym = symbol_create(ctx->allocator, name,
                                      SYMBOL_TYPE, node->super.location);
  sym->type.type = t;
  sym->state = SYMBOL_EVALUATED;
  scope_push_symbol(ctx->current_scope, sym);
  strmap_insert(ctx->type_name_table, name, t);

  /* Resolve fields and methods */
  _resolve_union_fields(ctx, t, node->members);
  context_evaluate_struct_union_members(ctx, t, node->members, 0);

  type_layout_compute(t, 8);
  type_hash_ensure(t);

  /* Check method bodies using unified helper */
  if (node->members) {
    size_t mcount = vec_get_size(node->members);
    for (size_t i = 0; i < mcount; i++) {
      node_t m = (node_t)vec_get(node->members, i);
      if (m->kind != CUBEC_NODE_STATEMENT_FUNCTION) continue;
      cubec_statement_function_t mfn = (cubec_statement_function_t)m;
      if (!mfn->body) continue;

      const char *mname = _checker_ident_str(mfn->name);
      semantic_type_t mtype = _find_method_type(ctx, t, mname);
      if (!mtype) continue;

      func_check_info_t minfo;
      func_check_info_from_statement(&minfo, mfn);
      _check_func_body_and_returns(ctx, &minfo,
                                   mtype->impl->function.return_type,
                                   mtype->impl->function.params,
                                   ctx->current_scope);
    }
  }
  return flow_state_alive(ctx->allocator);
}

/* --- local cunion declaration checker --- */

static flow_state_t _check_stmt_local_cunion(context_t ctx,
                                              cubec_statement_cunion_t node) {
  if (!node) return flow_state_alive(ctx->allocator);
  const char *name = _checker_ident_str(node->name);
  if (!name) return flow_state_alive(ctx->allocator);

  semantic_type_t t = semantic_type_create_named(ctx->allocator, name, TYPE_CUNION);
  vec_push(ctx->all_types, t);

  struct symbol *sym = symbol_create(ctx->allocator, name,
                                      SYMBOL_TYPE, node->super.location);
  sym->type.type = t;
  sym->state = SYMBOL_EVALUATED;
  scope_push_symbol(ctx->current_scope, sym);
  strmap_insert(ctx->type_name_table, name, t);

  _resolve_struct_fields(ctx, t, node->fields);

  type_layout_compute(t, 8);
  type_hash_ensure(t);
  return flow_state_alive(ctx->allocator);
}

/* --- local enum declaration checker --- */

static flow_state_t _check_stmt_local_enum(context_t ctx,
                                            cubec_statement_enum_t node) {
  if (!node) return flow_state_alive(ctx->allocator);
  const char *name = _checker_ident_str(node->name);
  if (!name) return flow_state_alive(ctx->allocator);

  semantic_type_t t = semantic_type_create_named(ctx->allocator, name, TYPE_ENUM);
  vec_push(ctx->all_types, t);

  struct symbol *sym = symbol_create(ctx->allocator, name,
                                      SYMBOL_TYPE, node->super.location);
  sym->type.type = t;
  sym->state = SYMBOL_EVALUATED;
  scope_push_symbol(ctx->current_scope, sym);
  strmap_insert(ctx->type_name_table, name, t);

  t->impl->enum_type.backing_type = ctx->builtin_i32;
  _resolve_enum_items(ctx, t, node->items);

  type_layout_compute(t, 8);
  type_hash_ensure(t);
  return flow_state_alive(ctx->allocator);
}

static void _check_function_body(context_t ctx,
                                  cubec_statement_function_t node) {
  if (!node || !node->body) return;

  const char *name = _checker_ident_str(node->name);
  struct symbol *sym = scope_lookup_local(ctx->global_scope, name);
  if (!sym || sym->kind != SYMBOL_FUNCTION || !sym->function.type) return;

  /* Skip generic functions — they are checked during monomorphization */
  if (sym->function.generic_params) return;

  /* Enqueue for body checking */
  _enqueue_body_check(ctx, sym, sym->function.type, NULL,
                       ctx->current_scope, false, NULL);
}

/* _register_func_params replaced by _register_func_params_from_info in checker_func_util.c */

static semantic_type_t _find_method_type(context_t ctx, semantic_type_t t,
                                          const char *mname) {
  (void)ctx;
  if (!t) return NULL;
  size_t mcount = vec_get_size(t->instance_methods);
  for (size_t j = 0; j < mcount; j++) {
    struct symbol *ms = (struct symbol *)vec_get(t->instance_methods, j);
    if (ms && ms->name && strcmp(ms->name, mname) == 0)
      return ms->function.type;
  }
  return NULL;
}

static void _check_type_method_bodies(context_t ctx, const char *type_name,
                                       vec_t members) {
  if (!members) return;
  semantic_type_t t = NULL;

  if (type_name) {
    struct symbol *sym = scope_lookup_local(ctx->global_scope, type_name);
    if (sym && sym->kind == SYMBOL_TYPE) t = sym->type.type;
  }

  size_t count = vec_get_size(members);
  for (size_t i = 0; i < count; i++) {
    node_t member = (node_t)vec_get(members, i);
    if (member->kind != CUBEC_NODE_STATEMENT_FUNCTION) continue;
    cubec_statement_function_t mfn = (cubec_statement_function_t)member;
    if (!mfn->body) continue;

    const char *mname = _checker_ident_str(mfn->name);
    semantic_type_t mtype = _find_method_type(ctx, t, mname);
    if (!mtype) continue;

    /* Skip generic methods — they are checked during monomorphization */
    func_check_info_t info;
    func_check_info_from_statement(&info, mfn);
    if (info.generic_params) continue;

    /* Find the method symbol for enqueuing */
    struct symbol *msym = NULL;
    if (t && t->instance_methods) {
      size_t mc = vec_get_size(t->instance_methods);
      for (size_t j = 0; j < mc; j++) {
        struct symbol *m = (struct symbol *)vec_get(t->instance_methods, j);
        if (m && m->name && strcmp(m->name, mname) == 0) {
          msym = m;
          break;
        }
      }
    }
    if (!msym) continue;

    _enqueue_body_check(ctx, msym, mtype, NULL,
                         ctx->global_scope, true, t);
  }
}

void context_check_function_bodies(context_t ctx, node_t program) {
  cubec_program_node_t prog = (cubec_program_node_t)program;
  if (!prog || !prog->statements) return;

  size_t count = vec_get_size(prog->statements);
  for (size_t i = 0; i < count; i++) {
    node_t stmt = (node_t)vec_get(prog->statements, i);
    if (!stmt) continue;

    switch (stmt->kind) {
    case CUBEC_NODE_STATEMENT_FUNCTION:
      _check_function_body(ctx, (cubec_statement_function_t)stmt);
      break;
    case CUBEC_NODE_STATEMENT_STRUCT: {
      cubec_statement_struct_t s = (cubec_statement_struct_t)stmt;
      _check_type_method_bodies(ctx, _checker_ident_str(s->name), s->members);
      break;
    }
    case CUBEC_NODE_STATEMENT_UNION: {
      cubec_statement_union_t u = (cubec_statement_union_t)stmt;
      _check_type_method_bodies(ctx, _checker_ident_str(u->name), u->members);
      break;
    }
    case CUBEC_NODE_STATEMENT_CUNION:
      /* C-style unions have no methods, nothing to check */
      break;
    default:
      break;
    }
  }
}

/* ===== worklist-driven body checking (Pass 3 + Pass 4) ===== */

void _enqueue_body_check(context_t ctx, struct symbol *func_sym,
                          semantic_type_t inst_type, strmap_t type_bindings,
                          scope_t scope_root, bool is_method,
                          semantic_type_t host_type) {
  const char *name = func_sym->name;

  /* Generate dedup key — for non-generic, use name directly (no allocation) */
  char *key = type_bindings
      ? _generic_instance_cache_key(ctx, name, type_bindings)
      : NULL;
  const char *lookup_key = key ? key : name;

  if (strmap_find(ctx->checked_bodies, lookup_key)) {
    if (key) allocator_free(ctx->allocator, &key);
    if (type_bindings) allocator_free(ctx->allocator, &type_bindings);
    return;
  }

  /* For non-generic: use name (persistent) as key — no allocation needed.
     For generic: use the heap-allocated key. */
  if (key) {
    strmap_insert(ctx->checked_bodies, key, (void *)(uintptr_t)1);
    allocator_free(ctx->allocator, &key);
  } else {
    /* name is persistent (from symbol table), safe to use as key */
    strmap_insert(ctx->checked_bodies, name, (void *)(uintptr_t)1);
  }

  body_check_entry_t *entry =
      allocator_alloc(ctx->allocator, sizeof(body_check_entry_t));
  if (!entry) {
    if (type_bindings) allocator_free(ctx->allocator, &type_bindings);
    return;
  }
  entry->func_sym = func_sym;
  entry->inst_type = inst_type;
  entry->type_bindings = type_bindings;
  entry->scope_root = scope_root;
  entry->is_method = is_method;
  entry->host_type = host_type;
  vec_push(ctx->body_check_worklist, entry);
}

static void _check_body_from_entry(context_t ctx, body_check_entry_t *entry) {
  struct symbol *sym = entry->func_sym;
  semantic_type_t inst_type = entry->inst_type;
  strmap_t type_bindings = entry->type_bindings;

  if (!sym || !sym->function.ast_node) goto done;
  cubec_statement_function_t fnode =
      (cubec_statement_function_t)sym->function.ast_node;
  if (!fnode->body) goto done;

  /* Create generic param bindings scope if type_bindings provided */
  scope_t scope_root = entry->scope_root;
  if (type_bindings) {
    scope_t generic_scope = scope_create(ctx->allocator, scope_root,
        SCOPE_BLOCK, fnode->super.location);
    vec_push(ctx->all_scopes, generic_scope);

    /* Bind all generic params from type_bindings (both type-level and method-level).
       With name-based bindings, no offset calculation needed — all names are unique. */
    strmap_iter_t iter = strmap_iter_first(type_bindings);
    const char *gp_name = NULL;
    while ((gp_name = strmap_iter_next(&iter)) != NULL) {
      semantic_type_t concrete = (semantic_type_t)strmap_find(type_bindings, gp_name);
      if (!gp_name || !concrete) continue;
      struct symbol *gp_sym = symbol_create(ctx->allocator,
          gp_name, SYMBOL_TYPE, fnode->super.location);
      gp_sym->type.type = concrete;
      gp_sym->state = SYMBOL_EVALUATED;
      scope_push_symbol(generic_scope, gp_sym);
    }
    scope_root = generic_scope;
  }

  {
    func_check_info_t info;
    func_check_info_from_statement(&info, fnode);

    _check_func_body_and_returns(ctx, &info,
        inst_type->impl->function.return_type,
        inst_type->impl->function.params,
        scope_root);
  }

done:
  if (type_bindings) allocator_free(ctx->allocator, &type_bindings);
}

void context_check_all_bodies(context_t ctx, node_t program) {
  if (!ctx || !program) return;

  /* Phase 1: Enqueue non-generic functions and methods */
  context_check_function_bodies(ctx, program);

  /* Phase 2: Process worklist — body checking may trigger new entries */
  size_t idx = 0;
  size_t max_iterations = vec_get_size(ctx->body_check_worklist) + 100; /* safety limit */
  while (idx < vec_get_size(ctx->body_check_worklist)) {
    if (idx >= max_iterations) {
      fprintf(stderr, "BUG: body check worklist exceeded safety limit (idx=%zu, size=%zu)\n",
              idx, vec_get_size(ctx->body_check_worklist));
      break;
    }
    body_check_entry_t *entry =
        (body_check_entry_t *)vec_get(ctx->body_check_worklist, idx);
    idx++;
    _check_body_from_entry(ctx, entry);
    allocator_free(ctx->allocator, &entry);
  }
}

/* ===== main entry ===== */

void context_check_program(context_t ctx, node_t program) {
  if (!ctx || !program) return;

  /* Pass 1: Declaration collection */
  context_collect_declarations(ctx, program);

  /* Pass 2: Sequential evaluation and checking */
  context_evaluate_declarations(ctx, program);

  /* Pass 3 + Pass 4: Body checking with worklist-driven generic monomorphization */
  context_check_all_bodies(ctx, program);

  /* Pass 5: Runtime collection — demand-driven diffusion from entry points */
  context_collect_runtime(ctx, program, false);
}
