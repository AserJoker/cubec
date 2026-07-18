#include "engine/comptime_eval_internal.h"
#include "engine/resolver.h"
#include "engine/symbol.h"
#include "core/allocator.h"
#include "core/string.h"
#include "core/vec.h"
#include "cubec/declaration_variable.h"
#include "cubec/expression_binary.h"
#include "cubec/expression_function.h"
#include "cubec/function_argument.h"
#include "cubec/statement_block.h"
#include "cubec/statement_if.h"
#include "cubec/statement_while.h"
#include "cubec/statement_for.h"
#include "cubec/statement_return.h"
#include "cubec/statement_foreach.h"
#include "cubec/statement_declaration.h"
#include "cubec/statement_function.h"
#include "cubec/statement_break.h"
#include "cubec/statement_continue.h"
#include "cubec/statement_empty.h"
#include "cubec/statement_defer.h"
#include "cubec/statement_switch.h"
#include "cubec/statement_expression.h"
#include "cubec/statement_comptime.h"
#include "cubec/statement_do_while.h"
#include "cubec/statement_foreach.h"
#include "cubec/switch_match.h"
#include <string.h>

/* --- defer execution --- */

static void _run_defers(comptime_eval_t eval, checker_t ctx,
                         size_t keep_count) {
  while (vec_get_size(eval->defer_stack) > keep_count) {
    size_t last = vec_get_size(eval->defer_stack) - 1;
    node_t defer_body = (node_t)vec_get(eval->defer_stack, last);
    vec_pop(eval->defer_stack);
    _comptime_exec_block(eval, ctx, defer_body);
    /* Do NOT free defer_body — it is owned by the AST tree */
  }
}

/* --- block execution --- */

comptime_signal_t _comptime_exec_block(comptime_eval_t eval, checker_t ctx,
                                        node_t block) {
  if (!block) return _eval_signal_none();
  cubec_statement_block_t blk = (cubec_statement_block_t)block;
  comptime_alloc_enter_scope(eval->valloc);

  comptime_env_t block_env = comptime_env_create(eval->allocator, eval->current_env);
  eval->current_env = block_env;

  size_t defer_base = vec_get_size(eval->defer_stack);
  comptime_signal_t sig = _eval_signal_none();

  if (blk->statements) {
    size_t count = vec_get_size(blk->statements);
    for (size_t i = 0; i < count; i++) {
      sig = _comptime_exec_stmt(eval, ctx, (node_t)vec_get(blk->statements, i));
      if (sig.kind != COMPTIME_SIGNAL_NONE) break;
    }
  }

  _run_defers(eval, ctx, defer_base);
  eval->current_env = block_env->parent;

  /* Clone return value into parent env before disposing block_env,
     since sig.return_value may be a borrowed ref from block_env's temporaries/bindings */
  if (sig.kind == COMPTIME_SIGNAL_RETURN && sig.return_value) {
    comptime_value_t cloned = comptime_value_clone(eval->allocator, sig.return_value);
    comptime_env_track_temp(eval->current_env, cloned);
    sig.return_value = cloned;
  }

  comptime_env_dispose(block_env);
  comptime_alloc_leave_scope(eval->valloc);

  return sig;
}

/* --- statement dispatcher --- */

comptime_signal_t _comptime_exec_stmt(comptime_eval_t eval, checker_t ctx,
                                       node_t stmt) {
  if (!stmt) return _eval_signal_none();

  switch (stmt->kind) {
  case CUBEC_NODE_STATEMENT_BLOCK:
    return _comptime_exec_block(eval, ctx, stmt);

  case CUBEC_NODE_STATEMENT_EXPRESSION: {
    cubec_statement_expression_t se = (cubec_statement_expression_t)stmt;
    _comptime_eval_expr(eval, ctx, se->expression);  /* result is temporary, auto-freed */
    return _eval_signal_none();
  }

  case CUBEC_NODE_STATEMENT_RETURN: {
    cubec_statement_return_t ret = (cubec_statement_return_t)stmt;
    comptime_value_t val = ret->expression
                               ? _comptime_eval_expr(eval, ctx, ret->expression)
                               : _eval_temp(eval, comptime_value_create_nil(eval->allocator, NULL));
    return _eval_signal_return(val);  /* borrowed — caller must clone before scope disposal */
  }

  case CUBEC_NODE_STATEMENT_IF: {
    cubec_statement_if_t si = (cubec_statement_if_t)stmt;
    comptime_value_t cond = _comptime_eval_expr(eval, ctx, si->condition);
    if (!cond || cond->kind == COMPTIME_VALUE_ERROR) return _eval_signal_error();
    comptime_signal_t result;
    if (comptime_value_is_truthy(cond)) {
      if (si->then_branch && si->then_branch->kind == CUBEC_NODE_STATEMENT_BLOCK)
        result = _comptime_exec_block(eval, ctx, si->then_branch);
      else
        result = _comptime_exec_stmt(eval, ctx, si->then_branch);
    } else if (si->else_branch) {
      if (si->else_branch->kind == CUBEC_NODE_STATEMENT_BLOCK)
        result = _comptime_exec_block(eval, ctx, si->else_branch);
      else
        result = _comptime_exec_stmt(eval, ctx, si->else_branch);
    } else {
      result = _eval_signal_none();
    }
    return result;
  }

  case CUBEC_NODE_STATEMENT_WHILE: {
    cubec_statement_while_t sw = (cubec_statement_while_t)stmt;
    eval->loop_depth++;
    int iterations = 0;
    comptime_signal_t sig = _eval_signal_none();
    while (true) {
      comptime_value_t cond = _comptime_eval_expr(eval, ctx, sw->condition);
      if (!cond || cond->kind == COMPTIME_VALUE_ERROR) {
        sig = _eval_signal_error();
        break;
      }
      if (!comptime_value_is_truthy(cond)) break;
      if (++iterations > COMPTIME_MAX_LOOP_ITERATIONS) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, stmt->location,
                             "comptime loop exceeded %d iterations",
                             COMPTIME_MAX_LOOP_ITERATIONS);
        ctx->error_count++;
        sig = _eval_signal_error();
        break;
      }
      sig = _comptime_exec_block(eval, ctx, sw->body);
      if (sig.kind == COMPTIME_SIGNAL_BREAK) {
        sig = _eval_signal_none();
        break;
      }
      if (sig.kind == COMPTIME_SIGNAL_CONTINUE) {
        sig = _eval_signal_none();
        continue;
      }
      if (sig.kind != COMPTIME_SIGNAL_NONE) break;
    }
    eval->loop_depth--;
    return sig;
  }

  case CUBEC_NODE_STATEMENT_FOR: {
    cubec_statement_for_t sf = (cubec_statement_for_t)stmt;
    comptime_alloc_enter_scope(eval->valloc);
    if (sf->init) _comptime_exec_stmt(eval, ctx, sf->init);
    eval->loop_depth++;
    int iterations = 0;
    comptime_signal_t sig = _eval_signal_none();
    while (true) {
      if (sf->condition) {
        comptime_value_t cond = _comptime_eval_expr(eval, ctx, sf->condition);
        if (!cond || cond->kind == COMPTIME_VALUE_ERROR) {
          sig = _eval_signal_error();
          break;
        }
        if (!comptime_value_is_truthy(cond)) break;
      }
      if (++iterations > COMPTIME_MAX_LOOP_ITERATIONS) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, stmt->location,
                             "comptime loop exceeded %d iterations",
                             COMPTIME_MAX_LOOP_ITERATIONS);
        ctx->error_count++;
        sig = _eval_signal_error();
        break;
      }
      sig = _comptime_exec_block(eval, ctx, sf->body);
      if (sig.kind == COMPTIME_SIGNAL_BREAK) {
        sig = _eval_signal_none();
        break;
      }
      if (sig.kind == COMPTIME_SIGNAL_CONTINUE) {
        sig = _eval_signal_none();
      } else if (sig.kind != COMPTIME_SIGNAL_NONE) {
        break;
      }
      if (sf->increment) _comptime_eval_expr(eval, ctx, sf->increment);
    }
    eval->loop_depth--;
    comptime_alloc_leave_scope(eval->valloc);
    return sig;
  }

  case CUBEC_NODE_STATEMENT_DO_WHILE: {
    cubec_statement_do_while_t sdw = (cubec_statement_do_while_t)stmt;
    eval->loop_depth++;
    int iterations = 0;
    comptime_signal_t sig = _eval_signal_none();
    while (true) {
      sig = _comptime_exec_block(eval, ctx, sdw->body);
      if (sig.kind == COMPTIME_SIGNAL_BREAK) {
        sig = _eval_signal_none();
        break;
      }
      if (sig.kind == COMPTIME_SIGNAL_CONTINUE) {
        sig = _eval_signal_none();
      } else if (sig.kind != COMPTIME_SIGNAL_NONE) {
        break;
      }
      comptime_value_t cond = _comptime_eval_expr(eval, ctx, sdw->condition);
      if (!cond || cond->kind == COMPTIME_VALUE_ERROR) {
        sig = _eval_signal_error();
        break;
      }
      if (!comptime_value_is_truthy(cond)) break;
      if (++iterations > COMPTIME_MAX_LOOP_ITERATIONS) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, stmt->location,
                             "comptime loop exceeded %d iterations",
                             COMPTIME_MAX_LOOP_ITERATIONS);
        ctx->error_count++;
        sig = _eval_signal_error();
        break;
      }
    }
    eval->loop_depth--;
    return sig;
  }

  case CUBEC_NODE_STATEMENT_FOREACH: {
    cubec_statement_foreach_t sfe = (cubec_statement_foreach_t)stmt;
    comptime_value_t iter = _comptime_eval_expr(eval, ctx, sfe->iterator);
    if (!iter || iter->kind == COMPTIME_VALUE_ERROR)
      return _eval_signal_error();

    /* foreach only supports iterators with a next() method.
     * Look up the next() instance method on the iterator's type. */
    if (!iter->type || !iter->type->instance_methods) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           stmt->location,
                           "foreach requires an iterator with next() method");
      ctx->error_count++;
      return _eval_signal_error();
    }

    struct symbol *next_sym = NULL;
    size_t mc = vec_get_size(iter->type->instance_methods);
    for (size_t i = 0; i < mc; i++) {
      struct symbol *s = (struct symbol *)vec_get(iter->type->instance_methods, i);
      if (s && s->name && strcmp(s->name, "next") == 0) {
        next_sym = s;
        break;
      }
    }
    if (!next_sym) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           stmt->location,
                           "foreach requires an iterator with next() method");
      ctx->error_count++;
      return _eval_signal_error();
    }

    /* Create the next() function value from the method symbol.
     * The symbol's ast_node holds the function body and param info. */
    comptime_value_t next_fn = _comptime_create_method_value(eval, ctx, next_sym);
    if (!next_fn || next_fn->kind != COMPTIME_VALUE_FUNCTION) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           stmt->location,
                           "foreach: next() method not found at comptime");
      ctx->error_count++;
      return _eval_signal_error();
    }

    /* Enter foreach scope */
    comptime_env_t loop_env =
        comptime_env_create(eval->allocator, eval->current_env);
    eval->current_env = loop_env;
    comptime_alloc_enter_scope(eval->valloc);

    eval->loop_depth++;
    int iterations = 0;
    comptime_signal_t sig = _eval_signal_none();

    while (true) {
      if (++iterations > COMPTIME_MAX_LOOP_ITERATIONS) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                             stmt->location,
                             "comptime loop exceeded %d iterations",
                             COMPTIME_MAX_LOOP_ITERATIONS);
        ctx->error_count++;
        sig = _eval_signal_error();
        break;
      }

      /* Call next() on the iterator: next(self=iter)
       * We manually manage the call env so we can copy the modified
       * self data back to the original iterator before disposing the env. */
      comptime_value_t self_arg = comptime_value_clone(eval->allocator, iter);

      comptime_env_t call_env =
          comptime_env_create(eval->allocator, next_fn->function.captured_env);
      /* Bind self parameter */
      if (next_fn->function.param_names) {
        size_t pcount = vec_get_size(next_fn->function.param_names);
        for (size_t pi = 0; pi < pcount; pi++) {
          const char *pname = (const char *)vec_get(next_fn->function.param_names, pi);
          if (pi == 0)
            comptime_env_bind_value(call_env, eval->valloc, pname, self_arg);
        }
      }

      comptime_env_t prev_env = eval->current_env;
      eval->current_env = call_env;
      eval->call_depth++;

      comptime_signal_t call_sig =
          _comptime_exec_block(eval, ctx, next_fn->function.body);

      eval->call_depth--;
      eval->current_env = prev_env;

      /* Copy modified self data back to original iterator.
       * The method may have mutated self (e.g. advancing a counter),
       * and those changes must persist for the next next() call. */
      if (self_arg && iter &&
          self_arg->kind == COMPTIME_VALUE_COMPOSITE &&
          iter->kind == COMPTIME_VALUE_COMPOSITE &&
          self_arg->composite.data_size == iter->composite.data_size &&
          self_arg->composite.data && iter->composite.data) {
        memcpy(iter->composite.data, self_arg->composite.data,
               iter->composite.data_size);
      }

      /* Clone return value before disposing call_env */
      comptime_value_t result = NULL;
      if (call_sig.kind == COMPTIME_SIGNAL_RETURN && call_sig.return_value) {
        result = comptime_value_clone(eval->allocator, call_sig.return_value);
        comptime_env_track_temp(eval->current_env, result);
      } else if (call_sig.kind == COMPTIME_SIGNAL_ERROR) {
        comptime_env_dispose(call_env);
        sig = _eval_signal_error();
        break;
      }

      comptime_env_dispose(call_env);

      if (!result || result->kind == COMPTIME_VALUE_ERROR) {
        sig = _eval_signal_error();
        break;
      }

      /* Check result.done */
      comptime_value_t done =
          comptime_value_get_field(result, "done", eval->allocator);
      if (done && comptime_value_is_truthy(done)) break;

      /* Get result.value */
      comptime_value_t value =
          comptime_value_get_field(result, "value", eval->allocator);
      if (!value) {
        sig = _eval_signal_error();
        break;
      }

      /* Bind/update loop variable */
      const char *vname = _eval_ident_str(sfe->variable);
      if (sfe->is_var_decl) {
        comptime_env_bind_value(loop_env, eval->valloc, vname, value);
      } else {
        comptime_env_update_value(loop_env, eval->valloc, vname, value);
      }

      /* Execute body */
      sig = _comptime_exec_block(eval, ctx, sfe->body);
      if (sig.kind == COMPTIME_SIGNAL_BREAK) {
        sig = _eval_signal_none();
        break;
      }
      if (sig.kind == COMPTIME_SIGNAL_CONTINUE) {
        sig = _eval_signal_none();
        continue;
      }
      if (sig.kind != COMPTIME_SIGNAL_NONE) break;
    }

    eval->loop_depth--;
    comptime_alloc_leave_scope(eval->valloc);
    /* Clone return value into parent env before disposing loop env */
    if (sig.kind == COMPTIME_SIGNAL_RETURN && sig.return_value) {
      comptime_value_t cloned =
          comptime_value_clone(eval->allocator, sig.return_value);
      comptime_env_track_temp(eval->current_env->parent, cloned);
      sig.return_value = cloned;
    }
    eval->current_env = loop_env->parent;
    comptime_env_dispose(loop_env);
    return sig;
  }

  case CUBEC_NODE_STATEMENT_DECLARATION: {
    cubec_statement_declaration_t sd = (cubec_statement_declaration_t)stmt;
    if (sd->declarator->kind != CUBEC_NODE_DECLARATION_VARIABLE)
      return _eval_signal_none();
    cubec_declaration_variable_t dv = (cubec_declaration_variable_t)sd->declarator;
    const char *name = _eval_ident_str(dv->identifier);
    if (!name) return _eval_signal_none();
    comptime_value_t val = dv->expression
                               ? _comptime_eval_expr(eval, ctx, dv->expression)
                               : NULL;
    if (!val || val->kind == COMPTIME_VALUE_ERROR) {
      /* No initializer — create a default value based on the variable's type */
      if (dv->type) {
        semantic_type_t var_type = resolver_resolve_type(ctx, dv->type);
        if (var_type && var_type->impl->kind != TYPE_ERROR) {
          if (var_type->impl->kind == TYPE_ARRAY) {
            val = _eval_temp(eval, comptime_value_create_composite(
                eval->allocator, var_type,
                var_type->impl->array.element,
                var_type->impl->size));
          } else if (var_type->impl->kind == TYPE_GENERIC_INSTANCE &&
                     var_type->impl->generic_instance.fields) {
            val = _eval_temp(eval, comptime_value_create_composite(
                eval->allocator, var_type,
                NULL, var_type->impl->size));
          } else {
            val = _eval_temp(eval, comptime_value_create_nil(eval->allocator, var_type));
          }
        }
      }
      if (!val)
        val = _eval_temp(eval, comptime_value_create_nil(eval->allocator, NULL));
    }
    if (val && val->kind != COMPTIME_VALUE_ERROR)
      comptime_env_bind_value(eval->current_env, eval->valloc, name, comptime_value_clone(eval->allocator, val));
    return _eval_signal_none();
  }

  case CUBEC_NODE_STATEMENT_FUNCTION: {
    cubec_statement_function_t sf = (cubec_statement_function_t)stmt;
    const char *name = _eval_ident_str(sf->name);
    if (!name) return _eval_signal_none();
    comptime_value_t val = _comptime_eval_expr(eval, ctx, (node_t)sf);
    if (val && val->kind != COMPTIME_VALUE_ERROR)
      comptime_env_bind_value(eval->current_env, eval->valloc, name, comptime_value_clone(eval->allocator, val));
    return _eval_signal_none();
  }

  case CUBEC_NODE_STATEMENT_BREAK:
    return eval->loop_depth > 0 ? _eval_signal_break() : _eval_signal_error();

  case CUBEC_NODE_STATEMENT_CONTINUE:
    return eval->loop_depth > 0 ? _eval_signal_continue() : _eval_signal_error();

  case CUBEC_NODE_STATEMENT_EMPTY:
    return _eval_signal_none();

  case CUBEC_NODE_STATEMENT_DEFER: {
    cubec_statement_defer_t sd = (cubec_statement_defer_t)stmt;
    vec_push(eval->defer_stack, sd->body);
    return _eval_signal_none();
  }

  case CUBEC_NODE_STATEMENT_SWITCH: {
    cubec_statement_switch_t ss = (cubec_statement_switch_t)stmt;
    comptime_value_t cond = _comptime_eval_expr(eval, ctx, ss->condition);
    if (!cond || cond->kind == COMPTIME_VALUE_ERROR) return _eval_signal_error();
    if (ss->matches) {
      size_t mc = vec_get_size(ss->matches);
      for (size_t i = 0; i < mc; i++) {
        cubec_switch_match_t arm =
            (cubec_switch_match_t)vec_get(ss->matches, i);
        if (arm->is_else)
          return _comptime_exec_block(eval, ctx, arm->body);
        if (arm->values) {
          size_t vc = vec_get_size(arm->values);
          for (size_t j = 0; j < vc; j++) {
            comptime_value_t v = _comptime_eval_expr(eval, ctx,
                (node_t)vec_get(arm->values, j));
            if (comptime_value_equals(cond, v))
              return _comptime_exec_block(eval, ctx, arm->body);
          }
        }
      }
    }
    return _eval_signal_none();
  }

  case CUBEC_NODE_STATEMENT_COMPTIME_BLOCK:
    return _comptime_exec_block(eval, ctx,
        ((cubec_statement_comptime_block_t)stmt)->body);

  case CUBEC_NODE_STATEMENT_COMPTIME_IF:
    return comptime_eval_exec_comptime_if(eval, ctx, stmt);

  case CUBEC_NODE_STATEMENT_COMPTIME_FOR:
    return comptime_eval_exec_comptime_for(eval, ctx, stmt);

  /* Type/import/test declarations: skip (handled by checker_evaluate) */
  case CUBEC_NODE_STATEMENT_STRUCT:
  case CUBEC_NODE_STATEMENT_ENUM:
  case CUBEC_NODE_STATEMENT_UNION:
  case CUBEC_NODE_STATEMENT_CUNION:
  case CUBEC_NODE_STATEMENT_INTERFACE:
  case CUBEC_NODE_STATEMENT_IMPORT:
  case CUBEC_NODE_STATEMENT_DECLARATION_TYPE:
  case CUBEC_NODE_STATEMENT_TEST:
    return _eval_signal_none();

  default:
    return _eval_signal_none();
  }
}

comptime_signal_t comptime_eval_exec_stmt(comptime_eval_t eval,
                                           checker_t ctx, node_t stmt) {
  return _comptime_exec_stmt(eval, ctx, stmt);
}

comptime_signal_t comptime_eval_exec_block(comptime_eval_t eval,
                                            checker_t ctx, node_t block) {
  return _comptime_exec_block(eval, ctx, block);
}

comptime_signal_t comptime_eval_exec_comptime_if(comptime_eval_t eval,
                                                  checker_t ctx,
                                                  node_t node) {
  cubec_statement_comptime_if_t ci = (cubec_statement_comptime_if_t)node;
  comptime_value_t cond = _comptime_eval_expr(eval, ctx, ci->condition);
  if (!cond || cond->kind == COMPTIME_VALUE_ERROR) return _eval_signal_error();
  if (comptime_value_is_truthy(cond)) {
    if (ci->then_branch && ci->then_branch->kind == CUBEC_NODE_STATEMENT_BLOCK)
      return _comptime_exec_block(eval, ctx, ci->then_branch);
    return _comptime_exec_stmt(eval, ctx, ci->then_branch);
  }
  if (ci->else_branch) {
    if (ci->else_branch->kind == CUBEC_NODE_STATEMENT_BLOCK)
      return _comptime_exec_block(eval, ctx, ci->else_branch);
    return _comptime_exec_stmt(eval, ctx, ci->else_branch);
  }
  return _eval_signal_none();
}

comptime_signal_t comptime_eval_exec_comptime_for(comptime_eval_t eval,
                                                   checker_t ctx,
                                                   node_t node) {
  cubec_statement_comptime_for_t cf = (cubec_statement_comptime_for_t)node;
  comptime_alloc_enter_scope(eval->valloc);
  if (cf->init) _comptime_exec_stmt(eval, ctx, cf->init);
  eval->loop_depth++;
  int iterations = 0;
  comptime_signal_t sig = _eval_signal_none();
  while (true) {
    if (cf->condition) {
      comptime_value_t cond = _comptime_eval_expr(eval, ctx, cf->condition);
      if (!cond || cond->kind == COMPTIME_VALUE_ERROR) {
        sig = _eval_signal_error();
        break;
      }
      if (!comptime_value_is_truthy(cond)) break;
    }
    if (++iterations > COMPTIME_MAX_LOOP_ITERATIONS) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR, node->location,
                           "comptime loop exceeded %d iterations",
                           COMPTIME_MAX_LOOP_ITERATIONS);
      ctx->error_count++;
      sig = _eval_signal_error();
      break;
    }
    sig = _comptime_exec_block(eval, ctx, cf->body);
    if (sig.kind == COMPTIME_SIGNAL_BREAK) {
      sig = _eval_signal_none();
      break;
    }
    if (sig.kind == COMPTIME_SIGNAL_CONTINUE) {
      sig = _eval_signal_none();
    } else if (sig.kind != COMPTIME_SIGNAL_NONE) {
      break;
    }
    if (cf->increment) _comptime_eval_expr(eval, ctx, cf->increment);
  }
  eval->loop_depth--;
  comptime_alloc_leave_scope(eval->valloc);
  return sig;
}
