#include "engine/checker.h"
#include "engine/checker_check_stmt.h"
#include "engine/checker_check_expr.h"
#include "engine/checker_type_util.h"
#include "engine/checker_collect.h"
#include "engine/checker_evaluate.h"
#include "engine/resolver.h"
#include "engine/symbol.h"
#include "core/allocator.h"
#include "core/string.h"
#include "core/vec.h"
#include "cubec/node.h"
#include "cubec/program.h"
#include "cubec/literal_identifier.h"
#include "cubec/statement_block.h"
#include "cubec/statement_expression.h"
#include "cubec/statement_return.h"
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

void _check_statement(checker_t ctx, node_t stmt,
                       semantic_type_t return_type);
static void _register_func_params(checker_t ctx,
                                    cubec_statement_function_t fn,
                                    vec_t params);

/* --- block --- */

static void _check_stmt_block(checker_t ctx, cubec_statement_block_t block,
                               semantic_type_t return_type) {
  if (!block || !block->statements) return;
  scope_t saved = ctx->current_scope;
  ctx->current_scope = scope_create(ctx->allocator, ctx->current_scope,
                                     SCOPE_BLOCK, block->super.location);
  vec_push(ctx->all_scopes, ctx->current_scope);
  size_t count = vec_get_size(block->statements);
  for (size_t i = 0; i < count; i++) {
    node_t s = (node_t)vec_get(block->statements, i);
    _check_statement(ctx, s, return_type);
  }
  ctx->current_scope = saved;
}

/* --- if --- */

static void _check_stmt_if(checker_t ctx, node_t stmt,
                            semantic_type_t return_type) {
  cubec_statement_if_t sif = (cubec_statement_if_t)stmt;
  semantic_type_t ct = _check_expression(ctx, sif->condition);
  if (!_is_bool_type(ct)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         stmt->location,
                         "if condition must be bool");
    ctx->error_count++;
  }
  _check_statement(ctx, sif->then_branch, return_type);
  if (sif->else_branch)
    _check_statement(ctx, sif->else_branch, return_type);
}

/* --- while --- */

static void _check_stmt_while(checker_t ctx, node_t stmt,
                               semantic_type_t return_type) {
  cubec_statement_while_t sw = (cubec_statement_while_t)stmt;
  semantic_type_t ct = _check_expression(ctx, sw->condition);
  if (!_is_bool_type(ct)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         stmt->location,
                         "while condition must be bool");
    ctx->error_count++;
  }
  ctx->loop_depth++;
  _check_statement(ctx, sw->body, return_type);
  ctx->loop_depth--;
}

/* --- do-while --- */

static void _check_stmt_do_while(checker_t ctx, node_t stmt,
                                  semantic_type_t return_type) {
  cubec_statement_do_while_t dw = (cubec_statement_do_while_t)stmt;
  ctx->loop_depth++;
  _check_statement(ctx, dw->body, return_type);
  ctx->loop_depth--;
  semantic_type_t ct = _check_expression(ctx, dw->condition);
  if (!_is_bool_type(ct)) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                         stmt->location,
                         "do-while condition must be bool");
    ctx->error_count++;
  }
}

/* --- for --- */

static void _check_stmt_for(checker_t ctx, node_t stmt,
                             semantic_type_t return_type) {
  cubec_statement_for_t sf = (cubec_statement_for_t)stmt;
  scope_t saved = ctx->current_scope;
  ctx->current_scope = scope_create(ctx->allocator, ctx->current_scope,
                                     SCOPE_FOR, stmt->location);
  vec_push(ctx->all_scopes, ctx->current_scope);
  if (sf->init) _check_statement(ctx, sf->init, return_type);
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
  ctx->loop_depth++;
  _check_statement(ctx, sf->body, return_type);
  ctx->loop_depth--;
  ctx->current_scope = saved;
}

/* --- foreach --- */

static void _check_stmt_foreach(checker_t ctx, node_t stmt,
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
  ctx->loop_depth++;
  _check_statement(ctx, sfe->body, return_type);
  ctx->loop_depth--;
  ctx->current_scope = saved;
}

/* --- switch --- */

static void _check_stmt_switch(checker_t ctx, node_t stmt,
                                semantic_type_t return_type) {
  cubec_statement_switch_t ss = (cubec_statement_switch_t)stmt;
  _check_expression(ctx, ss->condition);
  if (ss->matches) {
    size_t mcount = vec_get_size(ss->matches);
    for (size_t i = 0; i < mcount; i++) {
      node_t match = (node_t)vec_get(ss->matches, i);
      if (match->kind == CUBEC_NODE_SWITCH_MATCH) {
        cubec_switch_match_t sm = (cubec_switch_match_t)match;
        if (sm->values) {
          size_t vcount = vec_get_size(sm->values);
          for (size_t j = 0; j < vcount; j++) {
            _check_expression(ctx, (node_t)vec_get(sm->values, j));
          }
        }
        _check_statement(ctx, sm->body, return_type);
      }
    }
  }
}

/* --- declaration --- */

static void _check_stmt_declaration(checker_t ctx, node_t stmt) {
  cubec_statement_declaration_t sdecl =
      (cubec_statement_declaration_t)stmt;
  cubec_declaration_variable_t vdecl =
      (cubec_declaration_variable_t)sdecl->declarator;
  if (!vdecl) return;

  const char *vname = _checker_ident_str(vdecl->identifier);
  if (!vname) return;

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
  /* undefined initializer → TDZ; otherwise → EVALUATED */
  vsym->state = is_undefined_init ? SYMBOL_TDZ : SYMBOL_EVALUATED;
  scope_push_symbol(ctx->current_scope, vsym);
}

/* --- return --- */

static void _check_stmt_return(checker_t ctx, node_t stmt,
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
}

/* --- defer --- */

static void _check_stmt_defer(checker_t ctx, node_t stmt,
                               semantic_type_t return_type) {
  cubec_statement_defer_t sd = (cubec_statement_defer_t)stmt;
  _check_statement(ctx, sd->body, return_type);
}

/* --- comptime block --- */

static void _check_stmt_comptime_block(checker_t ctx, node_t stmt,
                                        semantic_type_t return_type) {
  /* Check body statements even though comptime evaluation is not yet implemented */
  cubec_statement_comptime_block_t cb =
      (cubec_statement_comptime_block_t)stmt;
  if (cb->body) _check_statement(ctx, cb->body, return_type);
}

/* --- comptime if --- */

static void _check_stmt_comptime_if(checker_t ctx, node_t stmt,
                                     semantic_type_t return_type) {
  cubec_statement_comptime_if_t ci =
      (cubec_statement_comptime_if_t)stmt;
  if (ci->condition) _check_expression(ctx, ci->condition);
  if (ci->then_branch) _check_statement(ctx, ci->then_branch, return_type);
  if (ci->else_branch) _check_statement(ctx, ci->else_branch, return_type);
}

/* --- comptime for --- */

static void _check_stmt_comptime_for(checker_t ctx, node_t stmt,
                                      semantic_type_t return_type) {
  cubec_statement_comptime_for_t cf =
      (cubec_statement_comptime_for_t)stmt;
  scope_t saved = ctx->current_scope;
  ctx->current_scope = scope_create(ctx->allocator, ctx->current_scope,
                                     SCOPE_COMPTIME, stmt->location);
  vec_push(ctx->all_scopes, ctx->current_scope);
  if (cf->init) _check_statement(ctx, cf->init, return_type);
  if (cf->condition) _check_expression(ctx, cf->condition);
  if (cf->increment) _check_expression(ctx, cf->increment);
  ctx->loop_depth++;
  if (cf->body) _check_statement(ctx, cf->body, return_type);
  ctx->loop_depth--;
  ctx->current_scope = saved;
}

/* --- statement dispatch --- */

static void _check_stmt_invalid_declaration(checker_t ctx, node_t stmt) {
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
}

static void _check_stmt_break_or_continue(checker_t ctx, node_t stmt,
                                           const char *keyword) {
  if (ctx->loop_depth <= 0) {
    diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, stmt->location,
                         "%s outside of loop", keyword);
    ctx->error_count++;
  }
}

void _check_statement(checker_t ctx, node_t stmt,
                      semantic_type_t return_type) {
  if (!stmt) return;
  switch (stmt->kind) {
  case CUBEC_NODE_STATEMENT_BLOCK:          _check_stmt_block(ctx, (cubec_statement_block_t)stmt, return_type); break;
  case CUBEC_NODE_STATEMENT_EXPRESSION: {
    semantic_type_t t = _check_expression(ctx, ((cubec_statement_expression_t)stmt)->expression);
    if (t && t->impl->kind != TYPE_ERROR && t->impl->kind != TYPE_VOID) {
      /* Check if the expression is a wildcard assignment: _ = expr */
      node_t inner = ((cubec_statement_expression_t)stmt)->expression;
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
    break;
  }
  case CUBEC_NODE_STATEMENT_RETURN:         _check_stmt_return(ctx, stmt, return_type); break;
  case CUBEC_NODE_STATEMENT_IF:             _check_stmt_if(ctx, stmt, return_type); break;
  case CUBEC_NODE_STATEMENT_WHILE:          _check_stmt_while(ctx, stmt, return_type); break;
  case CUBEC_NODE_STATEMENT_DO_WHILE:       _check_stmt_do_while(ctx, stmt, return_type); break;
  case CUBEC_NODE_STATEMENT_FOR:            _check_stmt_for(ctx, stmt, return_type); break;
  case CUBEC_NODE_STATEMENT_FOREACH:        _check_stmt_foreach(ctx, stmt, return_type); break;
  case CUBEC_NODE_STATEMENT_BREAK:          _check_stmt_break_or_continue(ctx, stmt, "break"); break;
  case CUBEC_NODE_STATEMENT_CONTINUE:       _check_stmt_break_or_continue(ctx, stmt, "continue"); break;
  case CUBEC_NODE_STATEMENT_DEFER:          _check_stmt_defer(ctx, stmt, return_type); break;
  case CUBEC_NODE_STATEMENT_SWITCH:         _check_stmt_switch(ctx, stmt, return_type); break;
  case CUBEC_NODE_STATEMENT_DECLARATION:    _check_stmt_declaration(ctx, stmt); break;
  case CUBEC_NODE_STATEMENT_EMPTY:          break;
  case CUBEC_NODE_STATEMENT_COMPTIME_BLOCK: _check_stmt_comptime_block(ctx, stmt, return_type); break;
  case CUBEC_NODE_STATEMENT_COMPTIME_IF:    _check_stmt_comptime_if(ctx, stmt, return_type); break;
  case CUBEC_NODE_STATEMENT_COMPTIME_FOR:   _check_stmt_comptime_for(ctx, stmt, return_type); break;
  default: _check_stmt_invalid_declaration(ctx, stmt); break;
  }
}

/* --- function body checker --- */

static void _check_function_body(checker_t ctx,
                                  cubec_statement_function_t node) {
  if (!node || !node->body) return;

  const char *name = _checker_ident_str(node->name);
  struct symbol *sym = scope_lookup_local(ctx->global_scope, name);
  if (!sym || sym->kind != SYMBOL_FUNCTION || !sym->function.type) return;

  /* Skip generic functions — they are checked during instantiation, not here */
  if (sym->function.generic_params) return;

  semantic_type_t ftype = sym->function.type;
  semantic_type_t return_type = ftype->impl->function.return_type;

  scope_t saved = ctx->current_scope;
  ctx->current_scope = scope_create(ctx->allocator, ctx->global_scope,
                                     SCOPE_FUNCTION, node->super.location);
  vec_push(ctx->all_scopes, ctx->current_scope);

  _register_func_params(ctx, node, ftype->impl->function.params);

  ctx->loop_depth = 0;
  _check_statement(ctx, node->body, return_type);

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
    _check_statement(ctx, mfn->body, return_type);
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
