#include "engine/checker.h"
#include "engine/checker_check_stmt.h"
#include "engine/checker_check_expr.h"
#include "engine/checker_type_util.h"
#include "engine/checker_collect.h"
#include "engine/checker_evaluate.h"
#include "engine/flow_state.h"
#include "engine/resolver.h"
#include "engine/symbol.h"
#include "engine/type_hash.h"
#include "engine/type_layout.h"
#include "engine/semantic_type.h"
#include "core/allocator.h"
#include "core/string.h"
#include "core/strmap.h"
#include "core/vec.h"
#include "cubec/node.h"
#include "cubec/program.h"
#include "cubec/literal_identifier.h"
#include "cubec/statement_block.h"
#include "cubec/function_capture.h"
#include "cubec/function_argument.h"
#include "cubec/statement_expression.h"
#include "cubec/statement_return.h"
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
#include "cubec/statement_function.h"
#include "cubec/statement_struct.h"
#include "cubec/statement_enum.h"
#include "cubec/statement_union.h"
#include "cubec/statement_cunion.h"
#include "cubec/statement_interface.h"
#include "cubec/statement_import.h"
#include "cubec/statement_test.h"
#include "cubec/switch_match.h"
#include "cubec/declaration_variable.h"
#include "cubec/function_argument.h"
#include "cubec/expression_assignment.h"
#include <string.h>

/* ===== Pass 3: Statement Checking ===== */

flow_state_t _check_statement(checker_t ctx, node_t stmt,
                               semantic_type_t return_type);
static void _register_func_params(checker_t ctx,
                                    cubec_statement_function_t fn,
                                    vec_t params);

/* --- block --- */

static flow_state_t _check_stmt_block(checker_t ctx,
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

static flow_state_t _check_stmt_if(checker_t ctx, node_t stmt,
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

static flow_state_t _check_stmt_while(checker_t ctx, node_t stmt,
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

static flow_state_t _check_stmt_do_while(checker_t ctx, node_t stmt,
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

static flow_state_t _check_stmt_for(checker_t ctx, node_t stmt,
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

static flow_state_t _check_stmt_foreach(checker_t ctx, node_t stmt,
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

static flow_state_t _check_stmt_switch(checker_t ctx, node_t stmt,
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

static flow_state_t _check_stmt_declaration(checker_t ctx, node_t stmt) {
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

  /* Return flow state with TDZ tracking */
  flow_state_t fs = flow_state_alive(ctx->allocator);
  if (is_undefined_init && vname) {
    flow_state_add_tdz(fs, vname);
  }
  return fs;
}

/* --- return --- */

static flow_state_t _check_stmt_return(checker_t ctx, node_t stmt,
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

static flow_state_t _check_stmt_defer(checker_t ctx, node_t stmt,
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

/* --- comptime block --- */

static flow_state_t _check_stmt_comptime_block(checker_t ctx, node_t stmt,
                                                semantic_type_t return_type) {
  cubec_statement_comptime_block_t cb =
      (cubec_statement_comptime_block_t)stmt;
  if (cb->body) {
    flow_state_t fs = _check_statement(ctx, cb->body, return_type);
    return fs;
  }
  if (ctx->current_flow)
    return flow_state_clone(ctx->allocator, ctx->current_flow);
  return flow_state_alive(ctx->allocator);
}

/* --- comptime if --- */

static flow_state_t _check_stmt_comptime_if(checker_t ctx, node_t stmt,
                                             semantic_type_t return_type) {
  cubec_statement_comptime_if_t ci =
      (cubec_statement_comptime_if_t)stmt;
  if (ci->condition) _check_expression(ctx, ci->condition);
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

/* --- comptime for --- */

static flow_state_t _check_stmt_comptime_for(checker_t ctx, node_t stmt,
                                              semantic_type_t return_type) {
  cubec_statement_comptime_for_t cf =
      (cubec_statement_comptime_for_t)stmt;
  scope_t saved = ctx->current_scope;
  ctx->current_scope = scope_create(ctx->allocator, ctx->current_scope,
                                     SCOPE_COMPTIME, stmt->location);
  vec_push(ctx->all_scopes, ctx->current_scope);
  if (cf->init) {
    flow_state_t init_fs = _check_statement(ctx, cf->init, return_type);
    flow_state_dispose(init_fs, ctx->allocator);
  }
  if (cf->condition) _check_expression(ctx, cf->condition);
  if (cf->increment) _check_expression(ctx, cf->increment);
  /* Save pre-loop flow state (body may not execute) */
  flow_state_t pre_flow = ctx->current_flow
      ? flow_state_clone(ctx->allocator, ctx->current_flow)
      : flow_state_alive(ctx->allocator);
  ctx->loop_depth++;
  flow_state_t body_fs;
  if (cf->body) {
    body_fs = _check_statement(ctx, cf->body, return_type);
  } else {
    body_fs = flow_state_alive(ctx->allocator);
  }
  ctx->loop_depth--;
  ctx->current_scope = saved;
  /* Loop body may not execute */
  flow_state_dispose(body_fs, ctx->allocator);
  return pre_flow;
}

/* --- local type declaration checkers (forward declarations) --- */

static flow_state_t _check_stmt_local_function(checker_t ctx,
                                                cubec_statement_function_t node);
static flow_state_t _check_stmt_local_struct(checker_t ctx,
                                              cubec_statement_struct_t node);
static flow_state_t _check_stmt_local_union(checker_t ctx,
                                             cubec_statement_union_t node);
static flow_state_t _check_stmt_local_cunion(checker_t ctx,
                                              cubec_statement_cunion_t node);
static flow_state_t _check_stmt_local_enum(checker_t ctx,
                                            cubec_statement_enum_t node);
static semantic_type_t _find_method_type(checker_t ctx, semantic_type_t t,
                                         const char *mname);

/* --- statement dispatch --- */

static flow_state_t _check_stmt_invalid_declaration(checker_t ctx, node_t stmt) {
  if (stmt->kind == CUBEC_NODE_STATEMENT_STRUCT ||
      stmt->kind == CUBEC_NODE_STATEMENT_ENUM ||
      stmt->kind == CUBEC_NODE_STATEMENT_UNION ||
      stmt->kind == CUBEC_NODE_STATEMENT_CUNION ||
      stmt->kind == CUBEC_NODE_STATEMENT_FUNCTION ||
      stmt->kind == CUBEC_NODE_STATEMENT_INTERFACE ||
      stmt->kind == CUBEC_NODE_STATEMENT_IMPORT ||
      stmt->kind == CUBEC_NODE_STATEMENT_TEST) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, stmt->location,
                         "declaration not allowed in this scope");
    ctx->error_count++;
  }
  return flow_state_alive(ctx->allocator);
}

static flow_state_t _check_stmt_break_or_continue(checker_t ctx, node_t stmt,
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

flow_state_t _check_statement(checker_t ctx, node_t stmt,
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
  case CUBEC_NODE_STATEMENT_COMPTIME_BLOCK:
    fs = _check_stmt_comptime_block(ctx, stmt, return_type);
    break;
  case CUBEC_NODE_STATEMENT_COMPTIME_IF:
    fs = _check_stmt_comptime_if(ctx, stmt, return_type);
    break;
  case CUBEC_NODE_STATEMENT_COMPTIME_FOR:
    fs = _check_stmt_comptime_for(ctx, stmt, return_type);
    break;
  default:
    fs = _check_stmt_invalid_declaration(ctx, stmt);
    break;
  }
  return fs;
}

/* --- function body checker --- */

static flow_state_t _check_stmt_local_function(checker_t ctx,
                                                cubec_statement_function_t node) {
  if (!node) return flow_state_alive(ctx->allocator);
  const char *name = _checker_ident_str(node->name);

  /* Resolve return type and parameter types */
  semantic_type_t ret_type = node->return_type
      ? resolver_resolve_type(ctx, node->return_type) : ctx->builtin_void;

  vec_init_t pvi = {.auto_dispose = false};
  vec_t param_types =
      (vec_t)allocator_create(ctx->allocator, &g_vec_type, &pvi);
  if (node->arguments) {
    size_t acount = vec_get_size(node->arguments);
    for (size_t i = 0; i < acount; i++) {
      node_t arg = (node_t)vec_get(node->arguments, i);
      if (arg->kind != CUBEC_NODE_FUNCTION_ARGUMENT) continue;
      cubec_function_argument_t farg = (cubec_function_argument_t)arg;
      semantic_type_t pt = farg->type
          ? resolver_resolve_type(ctx, farg->type) : ctx->error_type;
      vec_push(param_types, pt);
    }
  }

  semantic_type_t ftype = semantic_type_create_function(
      ctx->allocator, ret_type, param_types, node->is_c_variadic);
  type_hash_ensure(ftype);
  vec_push(ctx->all_types, ftype);

  /* Register function symbol in current scope */
  if (name) {
    struct symbol *sym = symbol_create(ctx->allocator, name,
                                        SYMBOL_FUNCTION, node->super.location);
    sym->function.type = ftype;
    sym->function.ast_node = (node_t)node;
    sym->state = SYMBOL_EVALUATED;
    scope_push_symbol(ctx->current_scope, sym);
  }

  /* Check function body */
  flow_state_t fs = flow_state_alive(ctx->allocator);
  if (node->body) {
    scope_t saved = ctx->current_scope;
    ctx->current_scope = scope_create(ctx->allocator, saved,
                                       SCOPE_FUNCTION, node->super.location);
    vec_push(ctx->all_scopes, ctx->current_scope);

    /* Register parameters */
    if (node->arguments) {
      size_t acount = vec_get_size(node->arguments);
      for (size_t i = 0; i < acount && i < vec_get_size(param_types); i++) {
        node_t arg = (node_t)vec_get(node->arguments, i);
        if (arg->kind != CUBEC_NODE_FUNCTION_ARGUMENT) continue;
        cubec_function_argument_t farg = (cubec_function_argument_t)arg;
        const char *pname = _checker_ident_str(farg->identifier);
        if (!pname) continue;
        struct symbol *psym = symbol_create(ctx->allocator, pname,
                                            SYMBOL_VARIABLE, arg->location);
        psym->variable.type = (semantic_type_t)vec_get(param_types, i);
        psym->variable.is_mutable = !semantic_type_is_const(psym->variable.type);
        psym->state = SYMBOL_EVALUATED;
        scope_push_symbol(ctx->current_scope, psym);
      }
    }

    /* Register captured variables */
    if (node->captures) {
      size_t cc = vec_get_size(node->captures);
      for (size_t i = 0; i < cc; i++) {
        node_t cap_node = (node_t)vec_get(node->captures, i);
        if (cap_node->kind != CUBEC_NODE_FUNCTION_CAPTURE) continue;
        cubec_function_capture_t cap = (cubec_function_capture_t)cap_node;
        const char *cap_name = _checker_ident_str(cap->identifier);
        if (!cap_name) continue;
        struct symbol *outer_sym = scope_lookup(saved, cap_name);
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

    ctx->loop_depth = 0;
    flow_state_dispose(fs, ctx->allocator);
    fs = _check_statement(ctx, node->body, ret_type);

    /* Return exhaustiveness check */
    if (ret_type->impl->kind != TYPE_VOID &&
        !flow_state_is_all_returned(fs)) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           node->super.location,
                           "non-void function must return a value on all paths");
      ctx->error_count++;
    }

    ctx->current_scope = saved;
  }
  return fs;
}

/* --- local struct declaration checker --- */

static flow_state_t _check_stmt_local_struct(checker_t ctx,
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

  /* Resolve fields */
  vec_init_t fvi = {.auto_dispose = true};
  vec_t fields = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &fvi);
  if (node->members) {
    size_t mcount = vec_get_size(node->members);
    for (size_t i = 0; i < mcount; i++) {
      node_t m = (node_t)vec_get(node->members, i);
      if (m->kind != CUBEC_NODE_STRUCT_FIELD) continue;
      cubec_struct_field_t sf = (cubec_struct_field_t)m;
      const char *fname = _checker_ident_str(sf->name);
      struct symbol *fsym = symbol_create(ctx->allocator, fname,
                                          SYMBOL_FIELD, sf->super.location);
      if (sf->type)
        fsym->field.type = resolver_resolve_type(ctx, sf->type);
      fsym->field.index = i;
      fsym->field.is_pub = sf->is_pub;
      vec_push(fields, fsym);
    }
  }
  t->impl->struct_type.fields = fields;
  allocator_free(ctx->allocator, &t->instance_methods);

  /* Compute layout */
  type_layout_compute(t, 8);
  type_hash_ensure(t);

  /* Check method bodies */
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

      scope_t saved = ctx->current_scope;
      ctx->current_scope = scope_create(ctx->allocator, saved,
                                         SCOPE_FUNCTION, mfn->super.location);
      vec_push(ctx->all_scopes, ctx->current_scope);
      _register_func_params(ctx, mfn, mtype->impl->function.params);
      ctx->loop_depth = 0;
      flow_state_t mfs = _check_statement(ctx, mfn->body, mtype->impl->function.return_type);
      /* Return exhaustiveness for method */
      if (mtype->impl->function.return_type->impl->kind != TYPE_VOID &&
          !flow_state_is_all_returned(mfs)) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                             mfn->super.location,
                             "non-void function must return a value on all paths");
        ctx->error_count++;
      }
      flow_state_dispose(mfs, ctx->allocator);
      ctx->current_scope = saved;
    }
  }
  return flow_state_alive(ctx->allocator);
}

/* --- local union declaration checker --- */

static flow_state_t _check_stmt_local_union(checker_t ctx,
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

  vec_init_t fvi = {.auto_dispose = true};
  vec_t fields = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &fvi);
  if (node->members) {
    size_t mcount = vec_get_size(node->members);
    for (size_t i = 0; i < mcount; i++) {
      node_t m = (node_t)vec_get(node->members, i);
      if (m->kind != CUBEC_NODE_UNION_FIELD) continue;
      cubec_union_field_t uf = (cubec_union_field_t)m;
      const char *fname = _checker_ident_str(uf->name);
      struct symbol *fsym = symbol_create(ctx->allocator, fname,
                                          SYMBOL_FIELD, uf->super.location);
      if (uf->type)
        fsym->field.type = resolver_resolve_type(ctx, uf->type);
      fsym->field.index = i;
      vec_push(fields, fsym);
    }
  }
  t->impl->struct_type.fields = fields;
  allocator_free(ctx->allocator, &t->instance_methods);

  type_layout_compute(t, 8);
  type_hash_ensure(t);

  /* Check method bodies (same as struct) */
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

      scope_t saved = ctx->current_scope;
      ctx->current_scope = scope_create(ctx->allocator, saved,
                                         SCOPE_FUNCTION, mfn->super.location);
      vec_push(ctx->all_scopes, ctx->current_scope);
      _register_func_params(ctx, mfn, mtype->impl->function.params);
      ctx->loop_depth = 0;
      flow_state_t mfs = _check_statement(ctx, mfn->body, mtype->impl->function.return_type);
      if (mtype->impl->function.return_type->impl->kind != TYPE_VOID &&
          !flow_state_is_all_returned(mfs)) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                             mfn->super.location,
                             "non-void function must return a value on all paths");
        ctx->error_count++;
      }
      flow_state_dispose(mfs, ctx->allocator);
      ctx->current_scope = saved;
    }
  }
  return flow_state_alive(ctx->allocator);
}

/* --- local cunion declaration checker --- */

static flow_state_t _check_stmt_local_cunion(checker_t ctx,
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

  vec_init_t fvi = {.auto_dispose = true};
  vec_t fields = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &fvi);
  if (node->fields) {
    size_t fcount = vec_get_size(node->fields);
    for (size_t i = 0; i < fcount; i++) {
      node_t f = (node_t)vec_get(node->fields, i);
      if (f->kind != CUBEC_NODE_STRUCT_FIELD) continue;
      cubec_struct_field_t sf = (cubec_struct_field_t)f;
      const char *fname = _checker_ident_str(sf->name);
      struct symbol *fsym = symbol_create(ctx->allocator, fname,
                                          SYMBOL_FIELD, sf->super.location);
      if (sf->type)
        fsym->field.type = resolver_resolve_type(ctx, sf->type);
      fsym->field.index = i;
      fsym->field.is_pub = sf->is_pub;
      vec_push(fields, fsym);
    }
  }
  t->impl->struct_type.fields = fields;

  type_layout_compute(t, 8);
  type_hash_ensure(t);
  return flow_state_alive(ctx->allocator);
}

/* --- local enum declaration checker --- */

static flow_state_t _check_stmt_local_enum(checker_t ctx,
                                            cubec_statement_enum_t node) {
  if (!node) return flow_state_alive(ctx->allocator);
  const char *name = _checker_ident_str(node->name);
  if (!name) return flow_state_alive(ctx->allocator);

  semantic_type_t t = semantic_type_create_named(ctx->allocator, name, TYPE_ENUM);
  vec_push(ctx->all_types, t);
  t->impl->enum_type.backing_type = ctx->builtin_i32;

  struct symbol *sym = symbol_create(ctx->allocator, name,
                                      SYMBOL_TYPE, node->super.location);
  sym->type.type = t;
  sym->state = SYMBOL_EVALUATED;
  scope_push_symbol(ctx->current_scope, sym);
  strmap_insert(ctx->type_name_table, name, t);

  /* Process enum items */
  vec_init_t ivi = {.auto_dispose = true};
  t->impl->enum_type.items = (vec_t)allocator_create(ctx->allocator, &g_vec_type, &ivi);

  int64_t auto_val = 0;
  if (node->items) {
    size_t icount = vec_get_size(node->items);
    for (size_t i = 0; i < icount; i++) {
      node_t item_node = (node_t)vec_get(node->items, i);
      if (item_node->kind != CUBEC_NODE_ENUM_ITEM) continue;
      cubec_enum_item_t ei = (cubec_enum_item_t)item_node;
      const char *iname = _checker_ident_str(ei->name);

      if (ei->value) {
        /* Try to evaluate the value expression as an integer */
        if (ei->value->kind == CUBEC_NODE_LITERAL_NUMERIC) {
          const char *numstr = string_get(((cubec_literal_numeric_t)ei->value)->value);
          if (numstr) auto_val = atoll(numstr);
        }
      }

      if (iname) {
        struct symbol *isym = symbol_create(ctx->allocator, iname,
                                            SYMBOL_ENUM_ITEM, item_node->location);
        isym->enum_item.value = auto_val;
        isym->enum_item.owning_type = t;
        isym->state = SYMBOL_EVALUATED;
        vec_push(t->impl->enum_type.items, isym);
      }
      auto_val++;
    }
  }

  type_layout_compute(t, 8);
  type_hash_ensure(t);
  return flow_state_alive(ctx->allocator);
}

static void _check_function_body(checker_t ctx,
                                  cubec_statement_function_t node) {
  if (!node || !node->body) return;

  const char *name = _checker_ident_str(node->name);
  struct symbol *sym = scope_lookup_local(ctx->global_scope, name);
  if (!sym || sym->kind != SYMBOL_FUNCTION || !sym->function.type) return;

  /* Skip generic function — they are checked during instantiation, not here */
  if (sym->function.generic_params) return;

  semantic_type_t ftype = sym->function.type;
  semantic_type_t return_type = ftype->impl->function.return_type;

  scope_t saved = ctx->current_scope;
  ctx->current_scope = scope_create(ctx->allocator, saved,
                                     SCOPE_FUNCTION, node->super.location);
  vec_push(ctx->all_scopes, ctx->current_scope);

  _register_func_params(ctx, node, ftype->impl->function.params);

  /* Register captured variables in function scope */
  if (node->captures) {
    size_t cc = vec_get_size(node->captures);
    for (size_t i = 0; i < cc; i++) {
      node_t cap_node = (node_t)vec_get(node->captures, i);
      if (cap_node->kind != CUBEC_NODE_FUNCTION_CAPTURE) continue;
      cubec_function_capture_t cap = (cubec_function_capture_t)cap_node;
      const char *cap_name = _checker_ident_str(cap->identifier);
      if (!cap_name) continue;
      /* Look up the captured variable in the enclosing scope */
      struct symbol *outer_sym = scope_lookup(saved, cap_name);
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

  ctx->loop_depth = 0;
  flow_state_t fs = _check_statement(ctx, node->body, return_type);

  /* Return exhaustiveness check */
  if (return_type->impl->kind != TYPE_VOID &&
      !flow_state_is_all_returned(fs)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         node->super.location,
                         "non-void function must return a value on all paths");
    ctx->error_count++;
  }

  flow_state_dispose(fs, ctx->allocator);
  ctx->current_flow = NULL;
  ctx->current_scope = saved;
}

static void _register_func_params(checker_t ctx, cubec_statement_function_t fn,
                                    vec_t params) {
  if (!fn->arguments) return;
  size_t acount = vec_get_size(fn->arguments);
  for (size_t j = 0; j < acount; j++) {
    node_t arg = (node_t)vec_get(fn->arguments, j);
    if (arg->kind != CUBEC_NODE_FUNCTION_ARGUMENT) continue;
    cubec_function_argument_t farg = (cubec_function_argument_t)arg;
    const char *pname = _checker_ident_str(farg->identifier);
    if (!pname) continue;
    struct symbol *psym = symbol_create(ctx->allocator, pname,
                                         SYMBOL_VARIABLE, arg->location);
    psym->variable.type = (params && j < vec_get_size(params))
                            ? (semantic_type_t)vec_get(params, j)
                            : ctx->error_type;
    psym->variable.is_mutable = !semantic_type_is_const(psym->variable.type);
    psym->state = SYMBOL_EVALUATED;
    scope_push_symbol(ctx->current_scope, psym);
  }
}

static semantic_type_t _find_method_type(checker_t ctx, semantic_type_t t,
                                          const char *mname) {
  if (!t) return NULL;
  size_t mcount = vec_get_size(t->instance_methods);
  for (size_t j = 0; j < mcount; j++) {
    struct symbol *ms = (struct symbol *)vec_get(t->instance_methods, j);
    if (ms && ms->name && strcmp(ms->name, mname) == 0)
      return ms->function.type;
  }
  return NULL;
}

static void _check_type_method_bodies(checker_t ctx, const char *type_name,
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
    semantic_type_t return_type = mtype
        ? mtype->impl->function.return_type
        : ctx->builtin_void;

    scope_t saved = ctx->current_scope;
    ctx->current_scope = scope_create(ctx->allocator, ctx->global_scope,
                                       SCOPE_FUNCTION, mfn->super.location);
    vec_push(ctx->all_scopes, ctx->current_scope);

    _register_func_params(ctx, mfn, mtype ? mtype->impl->function.params : NULL);

    ctx->loop_depth = 0;
    flow_state_t mfs = _check_statement(ctx, mfn->body, return_type);

    /* Return exhaustiveness for method */
    if (return_type->impl->kind != TYPE_VOID &&
        !flow_state_is_all_returned(mfs)) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           mfn->super.location,
                           "non-void function must return a value on all paths");
      ctx->error_count++;
    }

    flow_state_dispose(mfs, ctx->allocator);
    ctx->current_scope = saved;
  }
}

void checker_check_function_bodies(checker_t ctx, node_t program) {
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

/* ===== main entry ===== */

void checker_check_program(checker_t ctx, node_t program) {
  if (!ctx || !program) return;

  /* Pass 1: Declaration collection */
  checker_collect_declarations(ctx, program);

  /* Pass 2: Sequential evaluation and checking */
  checker_evaluate_declarations(ctx, program);

  /* Pass 3: Function body checking */
  checker_check_function_bodies(ctx, program);

  /* Pass 4: Generic instantiation — TODO */
}
