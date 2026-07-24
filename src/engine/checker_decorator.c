/**
 * @file checker_decorator.c
 * @brief Decorator semantic evaluation.
 *
 * Evaluates [[decorator]] and [[decorator(arg)]] on declarations.
 * Factory pattern: [[myDec(arg)]] evaluates myDec(arg) first,
 * then calls the returned function with the decorated item.
 */

#include "engine/checker_decorator.h"
#include "engine/comptime_eval.h"
#include "engine/comptime_eval_internal.h"
#include "engine/comptime_value.h"
#include "engine/diagnostic.h"
#include "engine/resolver.h"
#include "engine/symbol.h"
#include "engine/semantic_type.h"
#include "cubec/decorator.h"
#include "cubec/expression_call.h"
#include "cubec/literal_identifier.h"
#include "core/allocator.h"
#include "core/string.h"
#include "core/vec.h"
#include <stdio.h>
#include <string.h>

/* ===== helpers ===== */

static const char *_ident_str(node_t id_node) {
  if (!id_node || id_node->kind != CUBEC_NODE_LITERAL_IDENTIFIER) return NULL;
  return string_get(((cubec_literal_identifier_t)id_node)->value);
}

static const char *_value_kind_name(enum comptime_value_kind kind) {
  switch (kind) {
    case COMPTIME_VALUE_NIL:       return "nil";
    case COMPTIME_VALUE_BOOL:      return "bool";
    case COMPTIME_VALUE_INT:       return "int";
    case COMPTIME_VALUE_FLOAT:     return "float";
    case COMPTIME_VALUE_CHAR:      return "char";
    case COMPTIME_VALUE_STRING:    return "string";
    case COMPTIME_VALUE_TYPE:      return "type";
    case COMPTIME_VALUE_POINTER:   return "pointer";
    case COMPTIME_VALUE_COMPOSITE: return "composite";
    case COMPTIME_VALUE_FUNCTION:  return "function";
    case COMPTIME_VALUE_PACK:      return "pack";
    case COMPTIME_VALUE_ERROR:     return "error";
    case COMPTIME_VALUE_FATAL:     return "fatal";
    default:                       return "<unknown>";
  }
}

/**
 * @brief Get the comptime value of the decorated item.
 *
 * For VAR/FUNC: look up by name in comptime env.
 * For TYPE: create a COMPTIME_VALUE_TYPE wrapping the semantic type.
 */
static comptime_value_t _get_decorated_item(checker_t ctx,
                                             decorator_target_t target,
                                             const char *name,
                                             node_t ast_node) {
  comptime_eval_t eval = ctx->comptime_eval;
  if (!eval) return NULL;

  if (target == DECORATOR_TARGET_TYPE) {
    /* Look up the type by name in global scope */
    struct symbol *sym = scope_lookup(ctx->global_scope, name);
    semantic_type_t type = NULL;
    if (sym && sym->kind == SYMBOL_TYPE && sym->state != SYMBOL_TDZ) {
      type = sym->type.type;
    }
    if (!type) return NULL;
    comptime_value_t val = comptime_value_create_type(ctx->allocator, type);
    _eval_temp(eval, val);  /* track so it gets freed with env temporaries */
    return val;
  }

  /* VAR / FUNC: look up value in comptime env */
  comptime_env_t env = eval->current_env ? eval->current_env : eval->global_env;
  return comptime_env_lookup_value(env, eval->valloc, name);
}

/**
 * @brief Apply the decorator result to the decorated item.
 *
 * For FUNC: on first decorator that returns a function, bind the original
 * function value to `__original_<name>` in the comptime env, then update
 * the `<name>` binding to the decorator result. Also update the symbol's
 * ast_node and type to point to the decorator-returned function so that
 * codegen emits the wrapper as the primary function.
 *
 * For VAR: inline expansion — update the comptime env binding directly.
 *
 * For TYPE: in-place mutation — no binding update needed.
 */
static void _apply_decorator_result(checker_t ctx, decorator_target_t target,
                                     const char *name, node_t ast_node,
                                     comptime_value_t result,
                                     bool *original_saved) {
  comptime_eval_t eval = ctx->comptime_eval;
  if (!eval || !result || _val_is_error(result)) return;

  if (target == DECORATOR_TARGET_TYPE) {
    /* Type decorators return void — in-place mutation, nothing to update */
    return;
  }

  comptime_env_t env = eval->current_env ? eval->current_env : eval->global_env;

  if (target == DECORATOR_TARGET_FUNC && result->kind == COMPTIME_VALUE_FUNCTION) {
    /* On first decorator application, save the original function */
    if (!*original_saved) {
      comptime_value_t orig = comptime_env_lookup_value(env, eval->valloc, name);
      if (orig) {
        /* Bind original as __original_<name> */
        char orig_name[256];
        snprintf(orig_name, sizeof(orig_name), "__original_%s", name);
        comptime_env_bind_value(env, eval->valloc, orig_name,
                                comptime_value_clone(ctx->allocator, orig));
        *original_saved = true;
      }
    }

    /* Update comptime env binding to the decorator result */
    comptime_env_update_value(env, eval->valloc, name,
                              comptime_value_clone(ctx->allocator, result));

    /* Note: symbol ast_node/type update deferred to codegen phase.
     * The comptime env binding is sufficient for compile-time dispatch.
     * At codegen, the original function will be emitted as __original_<name>
     * and the decorator-returned wrapper will be emitted as <name>. */
    return;
  }

  /* VAR / FUNC (non-function result): update the comptime env binding */
  if (target == DECORATOR_TARGET_VAR || target == DECORATOR_TARGET_FUNC) {
    comptime_env_update_value(env, eval->valloc, name,
                              comptime_value_clone(ctx->allocator, result));
  }
}

/* ===== public API ===== */

void checker_evaluate_decorators(checker_t ctx, vec_t decorators,
                                  decorator_target_t target,
                                  const char *name, node_t ast_node) {
  if (!ctx || !decorators || !name) return;
  comptime_eval_t eval = ctx->comptime_eval;
  if (!eval) return;

  bool original_saved = false;  /* tracks if __original_<name> was bound */
  size_t dcount = vec_get_size(decorators);
  for (size_t i = 0; i < dcount; i++) {
    cubec_decorator_t dec = (cubec_decorator_t)vec_get(decorators, i);
    if (!dec || !dec->expression) continue;

    node_t expr = dec->expression;
    comptime_value_t dec_fn = NULL;

    if (expr->kind == CUBEC_NODE_EXPRESSION_CALL) {
      /* [[myDec(arg)]] → factory pattern:
       * 1. Evaluate myDec(arg) → must return a comptime function
       * 2. Call the returned function with the decorated item */
      dec_fn = _comptime_eval_expr(eval, ctx, expr);
      if (_val_is_error(dec_fn)) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                             expr->location,
                             "decorator expression evaluation failed");
        ctx->error_count++;
        continue;
      }
      if (dec_fn->kind != COMPTIME_VALUE_FUNCTION) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                             expr->location,
                             "decorator factory must return a comptime function, got '%s'",
                             _value_kind_name(dec_fn->kind));
        ctx->error_count++;
        continue;
      }
    } else {
      /* [[myDec]] → direct decorator:
       * Evaluate the expression → must be a comptime function */
      dec_fn = _comptime_eval_expr(eval, ctx, expr);
      if (_val_is_error(dec_fn)) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                             expr->location,
                             "unknown decorator '%s'",
                             expr->kind == CUBEC_NODE_LITERAL_IDENTIFIER
                                 ? _ident_str(expr) : "<expression>");
        ctx->error_count++;
        continue;
      }
      if (dec_fn->kind != COMPTIME_VALUE_FUNCTION) {
        diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                             expr->location,
                             "decorator must resolve to a comptime function, got '%s'",
                             _value_kind_name(dec_fn->kind));
        ctx->error_count++;
        continue;
      }
    }

    /* Get the decorated item's comptime value */
    comptime_value_t item = _get_decorated_item(ctx, target, name, ast_node);
    if (!item) {
      diagnostic_list_push(ctx->diagnostics, DIAGNOSTIC_ERROR,
                           expr->location,
                           "cannot evaluate decorator on '%s': item not available at comptime",
                           name);
      ctx->error_count++;
      continue;
    }

    /* Clone the item — _eval_call_function takes ownership of args */
    comptime_value_t item_clone = comptime_value_clone(eval->allocator, item);

    /* Call the decorator function with the decorated item.
     * _eval_call_function already tracks the return value in current_env->temporaries. */
    comptime_value_t result = _eval_call_function(eval, ctx, dec_fn, &item_clone, 1,
                                                    (node_t)dec);

    /* Apply the result */
    if (result && !_val_is_error(result)) {
      _apply_decorator_result(ctx, target, name, ast_node, result,
                              &original_saved);
    }
  }
}
