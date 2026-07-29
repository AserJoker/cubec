#include "c/lower.h"
#include "c/mangle.h"
#include "c/c_ir_function.h"
#include "c/c_ir_variable.h"
#include "c/c_ir_enum.h"
#include "c/c_ir_typedef.h"
#include "c/c_ir_forward_decl.h"
#include "c/c_ir_include.h"
#include "c/c_ir_stmt_block.h"
#include "c/c_ir_stmt_expr.h"
#include "c/c_ir_stmt_return.h"
#include "c/c_ir_stmt_if.h"
#include "c/c_ir_stmt_while.h"
#include "c/c_ir_stmt_do_while.h"
#include "c/c_ir_stmt_for.h"
#include "c/c_ir_stmt_jump.h"
#include "c/c_ir_stmt_local_decl.h"
#include "c/c_ir_stmt_stmt_expr.h"
#include "c/c_ir_expr_binary.h"
#include "c/c_ir_expr_literal.h"
#include "c/c_ir_expr_call.h"
#include "c/c_ir_expr_member.h"
#include "cubec/node.h"
#include "cubec/statement_block.h"
#include "cubec/statement_return.h"
#include "cubec/statement_if.h"
#include "cubec/statement_while.h"
#include "cubec/statement_do_while.h"
#include "cubec/statement_for.h"
#include "cubec/statement_declaration.h"
#include "cubec/statement_function.h"
#include "cubec/statement_enum.h"
#include "cubec/statement_struct.h"
#include "cubec/statement_union.h"
#include "cubec/statement_cunion.h"
#include "cubec/statement_defer.h"
#include "cubec/statement_switch.h"
#include "cubec/switch_match.h"
#include "cubec/statement_foreach.h"
#include "cubec/statement_expression.h"
#include "cubec/expression_binary.h"
#include "cubec/declaration_variable.h"
#include "cubec/literal_identifier.h"
#include "engine/resolver.h"
#include <string.h>

/* ===== Scope management helpers ===== */

/**
 * @brief Enter a new block scope, save/restore ctx->current_scope.
 * Called at the start of block/for/if/while processing.
 */
static scope_t enter_scope(context_t ctx, location_t loc) {
  scope_t saved = ctx->current_scope;
  ctx->current_scope = scope_create(ctx->allocator, saved, SCOPE_BLOCK, loc);
  vec_push(ctx->all_scopes, ctx->current_scope);
  return saved;
}

static void leave_scope(context_t ctx, scope_t saved) {
  ctx->current_scope = saved;
}

/* Forward declarations */
c_type_t lower_type(allocator_t allocator, semantic_type_t type,
                     const char *module_hash, c_ir_unit_t unit);
semantic_type_t lower_infer_type(allocator_t allocator, context_t ctx,
                                   node_t node, const char *module_hash,
                                   c_ir_unit_t unit);
c_ir_node_t lower_expr(allocator_t allocator, context_t ctx, node_t node,
                        const char *module_hash, c_ir_unit_t unit);

/**
 * @brief Lower a cubec statement AST node to a C IR statement node.
 * @param defer_stack  Vector of defer AST nodes (LIFO order), may be NULL
 */
c_ir_node_t lower_stmt(allocator_t allocator, context_t ctx, node_t node,
                        const char *module_hash, vec_t defer_stack, c_ir_unit_t unit) {
  if (!node) return NULL;

  /* Error nodes — skip */
  if (node->kind == CUBEC_NODE_ERROR || node->kind == CUBEC_NODE_STATEMENT_ERROR) {
    return NULL;
  }

  location_t loc = node->location;

  switch (node->kind) {

  /* ===== Block ===== */
  case CUBEC_NODE_STATEMENT_BLOCK: {
    cubec_statement_block_t n = (cubec_statement_block_t)node;
    scope_t saved = enter_scope(ctx, loc);
    vec_t stmts = allocator_create(allocator, &g_vec_type,
                                    &(vec_init_t){.auto_dispose = false});
    size_t count = n->statements ? vec_get_size(n->statements) : 0;
    for (size_t i = 0; i < count; i++) {
      node_t child = vec_get(n->statements, i);
      c_ir_node_t c_child = lower_stmt(allocator, ctx, child, module_hash, defer_stack, unit);
      if (c_child) vec_push(stmts, c_child);
    }
    leave_scope(ctx, saved);
    return (c_ir_node_t)c_ir_stmt_block_create(allocator, stmts, loc);
  }

  /* ===== Expression statement ===== */
  case CUBEC_NODE_STATEMENT_EXPRESSION: {
    cubec_statement_expression_t n = (cubec_statement_expression_t)node;
    c_ir_node_t expr = lower_expr(allocator, ctx, n->expression, module_hash, NULL);
    if (!expr) return NULL;
    /* If the expression is itself a C_IR_STMT_STMT_EXPR (e.g., from .? or .!),
     * unwrap it to avoid double-wrapping as expression statement */
    if (c_ir_get_kind(expr) == C_IR_STMT_STMT_EXPR) {
      return expr;
    }
    return (c_ir_node_t)c_ir_stmt_expr_create(allocator, expr, loc);
  }

  /* ===== Return ===== */
  case CUBEC_NODE_STATEMENT_RETURN: {
    cubec_statement_return_t n = (cubec_statement_return_t)node;
    c_ir_node_t value = n->expression
        ? lower_expr(allocator, ctx, n->expression, module_hash, NULL)
        : NULL;

    /* Insert defer cleanup before return (LIFO order) */
    if (defer_stack && vec_get_size(defer_stack) > 0) {
      vec_t stmts = allocator_create(allocator, &g_vec_type,
                                       &(vec_init_t){.auto_dispose = false});
      size_t defer_count = vec_get_size(defer_stack);
      for (size_t i = defer_count; i > 0; i--) {
        node_t defer_node = vec_get(defer_stack, i - 1);
        cubec_statement_defer_t d = (cubec_statement_defer_t)defer_node;
        c_ir_node_t defer_body = lower_stmt(allocator, ctx, d->body, module_hash, NULL, unit);
        if (defer_body) vec_push(stmts, defer_body);
      }
      c_ir_node_t ret = (c_ir_node_t)c_ir_stmt_return_create(allocator, value, loc);
      vec_push(stmts, ret);
      return (c_ir_node_t)c_ir_stmt_block_create(allocator, stmts, loc);
    }

    return (c_ir_node_t)c_ir_stmt_return_create(allocator, value, loc);
  }

  /* ===== If ===== */
  case CUBEC_NODE_STATEMENT_IF: {
    cubec_statement_if_t n = (cubec_statement_if_t)node;
    c_ir_node_t cond = lower_expr(allocator, ctx, n->condition, module_hash, NULL);
    c_ir_node_t then_b = lower_stmt(allocator, ctx, n->then_branch, module_hash, defer_stack, unit);
    c_ir_node_t else_b = n->else_branch
        ? lower_stmt(allocator, ctx, n->else_branch, module_hash, defer_stack, unit)
        : NULL;
    return (c_ir_node_t)c_ir_stmt_if_create(allocator, cond, then_b, else_b, loc);
  }

  /* ===== While ===== */
  case CUBEC_NODE_STATEMENT_WHILE: {
    cubec_statement_while_t n = (cubec_statement_while_t)node;
    c_ir_node_t cond = lower_expr(allocator, ctx, n->condition, module_hash, NULL);
    c_ir_node_t body = lower_stmt(allocator, ctx, n->body, module_hash, defer_stack, unit);
    return (c_ir_node_t)c_ir_stmt_while_create(allocator, cond, body, loc);
  }

  /* ===== Do-while ===== */
  case CUBEC_NODE_STATEMENT_DO_WHILE: {
    cubec_statement_do_while_t n = (cubec_statement_do_while_t)node;
    c_ir_node_t body = lower_stmt(allocator, ctx, n->body, module_hash, defer_stack, unit);
    c_ir_node_t cond = lower_expr(allocator, ctx, n->condition, module_hash, NULL);
    return (c_ir_node_t)c_ir_stmt_do_while_create(allocator, body, cond, loc);
  }

  /* ===== For ===== */
  case CUBEC_NODE_STATEMENT_FOR: {
    cubec_statement_for_t n = (cubec_statement_for_t)node;
    c_ir_node_t init = n->init ? lower_stmt(allocator, ctx, n->init, module_hash, defer_stack, unit) : NULL;
    c_ir_node_t cond = n->condition ? lower_expr(allocator, ctx, n->condition, module_hash, NULL) : NULL;
    c_ir_node_t update = n->increment ? lower_expr(allocator, ctx, n->increment, module_hash, NULL) : NULL;
    c_ir_node_t body = lower_stmt(allocator, ctx, n->body, module_hash, defer_stack, unit);
    return (c_ir_node_t)c_ir_stmt_for_create(allocator, init, cond, update, body, loc);
  }

  /* ===== Break / Continue ===== */
  case CUBEC_NODE_STATEMENT_BREAK:
    return (c_ir_node_t)c_ir_stmt_break_create(allocator, loc);

  case CUBEC_NODE_STATEMENT_CONTINUE:
    return (c_ir_node_t)c_ir_stmt_continue_create(allocator, loc);

  /* ===== Declaration ===== */
  case CUBEC_NODE_STATEMENT_DECLARATION: {
    cubec_statement_declaration_t n = (cubec_statement_declaration_t)node;
    /* Comptime/builtin declarations produce no C code */
    if (n->is_comptime || n->is_builtin) return NULL;

    cubec_declaration_variable_t var = (cubec_declaration_variable_t)n->declarator;
    if (!var || !var->identifier) return NULL;

    const char *name = string_get(
        ((cubec_literal_identifier_t)var->identifier)->value);

    /* Resolve type using lower's own type inference:
     * 1. Explicit type annotation (var x: T) via resolver_resolve_type
     * 2. Scope lookup in lower's scope chain (for variables already registered)
     * 3. Expression type inference via lower_infer_type
     */
    c_type_t c_type = NULL;
    bool is_mutable = true;
    semantic_type_t sem_type = NULL;

    /* 1. Explicit type annotation */
    if (var->type) {
      sem_type = resolver_resolve_type(ctx, var->type);
    }

    /* 2. Scope lookup in lower's own scope chain */
    if (!sem_type || sem_type->impl->kind == TYPE_ERROR) {
      struct symbol *sym = scope_lookup(ctx->current_scope, name);
      if (sym && sym->kind == SYMBOL_VARIABLE && sym->variable.type) {
        sem_type = sym->variable.type;
        is_mutable = sym->variable.is_mutable;
      }
    }

    /* 3. Expression type inference via lower_infer_type */
    if ((!sem_type || sem_type->impl->kind == TYPE_ERROR) && var->expression) {
      sem_type = lower_infer_type(allocator, ctx, var->expression, module_hash, unit);
    }

    if (sem_type && sem_type->impl->kind != TYPE_ERROR) {
      c_type = lower_type(allocator, sem_type, module_hash, unit);
      /* Create a symbol and push into current scope so nested lookups find it.
       * This handles shadowing: inner `var x:bool` gets its own symbol
       * that shadows the outer `var x:i32`. */
      struct symbol *existing = scope_lookup_local(ctx->current_scope, name);
      if (!existing) {
        struct symbol *sym = symbol_create(ctx->allocator, name, SYMBOL_VARIABLE, loc);
        sym->variable.type = sem_type;
        sym->variable.is_mutable = is_mutable;
        sym->variable.is_comptime = false;
        sym->state = SYMBOL_EVALUATED;
        scope_push_symbol(ctx->current_scope, sym);
      }
    }

    if (!c_type) {
      c_type = c_type_primitive(allocator, "void");
    }

    if (!is_mutable) c_type_const(allocator, c_type);

    c_ir_node_t init = var->expression
        ? lower_expr(allocator, ctx, var->expression, module_hash, NULL)
        : NULL;

    return (c_ir_node_t)c_ir_stmt_local_decl_create(allocator, c_type, name,
                                                       init, loc);
  }

  /* ===== Function ===== */
  case CUBEC_NODE_STATEMENT_FUNCTION: {
    /* Function definitions are handled in lower.c via symbol table.
     * Nested functions use EXPRESSION_FUNCTION (anonymous function) syntax.
     * Extern declarations inside function bodies don't need C IR output
     * since they're already in the global scope. */
    return NULL;
  }

  /* ===== Struct ===== */
  case CUBEC_NODE_STATEMENT_STRUCT: {
    /* Structs generate typedefs — handled in lower_program */
    return NULL;
  }

  /* ===== Enum ===== */
  case CUBEC_NODE_STATEMENT_ENUM: {
    /* Enums generate enum defs — handled in lower_program */
    return NULL;
  }

  /* ===== Union / CUnion ===== */
  case CUBEC_NODE_STATEMENT_UNION:
  case CUBEC_NODE_STATEMENT_CUNION: {
    return NULL;
  }

  /* ===== Defer ===== */
  case CUBEC_NODE_STATEMENT_DEFER: {
    /* Collect defer into the stack — will be emitted before return or
     * at end of function body */
    if (defer_stack) {
      vec_push(defer_stack, node);
    }
    return NULL;
  }

  /* ===== Switch ===== */
  case CUBEC_NODE_STATEMENT_SWITCH: {
    cubec_statement_switch_t n = (cubec_statement_switch_t)node;
    size_t match_count = n->matches ? vec_get_size(n->matches) : 0;
    if (match_count == 0) return NULL;

    /* Lower the switch condition and cache it in a local variable */
    c_ir_node_t cond_expr = lower_expr(allocator, ctx, n->condition, module_hash, NULL);
    if (!cond_expr) return NULL;

    /* Build if-else chain from last match to first (reverse) */
    c_ir_node_t else_branch = NULL;

    for (size_t i = match_count; i > 0; i--) {
      cubec_switch_match_t m = vec_get(n->matches, i - 1);
      c_ir_node_t then = lower_stmt(allocator, ctx, m->body, module_hash, defer_stack, unit);
      if (!then) then = (c_ir_node_t)c_ir_stmt_block_create(
          allocator,
          allocator_create(allocator, &g_vec_type,
                           &(vec_init_t){.auto_dispose = false}),
          loc);

      if (m->is_else) {
        else_branch = then;
        continue;
      }

      /* Build condition: _switch_val == v1 || _switch_val == v2 || ... */
      size_t val_count = m->values ? vec_get_size(m->values) : 0;
      c_ir_node_t if_cond = NULL;

      for (size_t j = 0; j < val_count; j++) {
        c_ir_node_t val = lower_expr(allocator, ctx,
                                       vec_get(m->values, j), module_hash, NULL);
        c_ir_node_t cond_ref = (c_ir_node_t)c_ir_expr_ident_create(
            allocator, "_switch_val", loc);
        c_ir_node_t eq = (c_ir_node_t)c_ir_expr_binary_create(
            allocator, "==", cond_ref, val, loc);
        if (!if_cond) {
          if_cond = eq;
        } else {
          if_cond = (c_ir_node_t)c_ir_expr_binary_create(
              allocator, "||", if_cond, eq, loc);
        }
      }

      if (if_cond) {
        else_branch = (c_ir_node_t)c_ir_stmt_if_create(
            allocator, if_cond, then, else_branch, loc);
      }
    }

    /* Wrap in block with local variable: { auto _switch_val = cond; if-else chain } */
    vec_t outer = allocator_create(allocator, &g_vec_type,
                                     &(vec_init_t){.auto_dispose = false});
    c_type_t cond_type = c_type_primitive(allocator, "void"); /* placeholder */
    c_ir_node_t cond_decl = (c_ir_node_t)c_ir_stmt_local_decl_create(
        allocator, cond_type, "_switch_val", cond_expr, loc);
    vec_push(outer, cond_decl);
    if (else_branch) vec_push(outer, else_branch);

    return (c_ir_node_t)c_ir_stmt_block_create(allocator, outer, loc);
  }

  /* ===== Foreach ===== */
  case CUBEC_NODE_STATEMENT_FOREACH: {
    cubec_statement_foreach_t n = (cubec_statement_foreach_t)node;
    if (!n->variable || !n->iterator) return NULL;

    const char *item_name = string_get(
        ((cubec_literal_identifier_t)n->variable)->value);

    /* Build: {
     *   _iter = collection;
     *   while (true) {
     *     _result = _iter.next();
     *     if (_result.done) break;
     *     item = _result.value;
     *     body;
     *   }
     * }
     */
    vec_t outer_stmts = allocator_create(allocator, &g_vec_type,
                                          &(vec_init_t){.auto_dispose = false});

    /* _iter = collection */
    c_ir_node_t iter_init = lower_expr(allocator, ctx, n->iterator, module_hash, NULL);
    c_type_t iter_type = c_type_primitive(allocator, "void"); /* placeholder */
    c_ir_node_t iter_decl = (c_ir_node_t)c_ir_stmt_local_decl_create(
        allocator, iter_type, "_iter", iter_init, loc);
    vec_push(outer_stmts, iter_decl);

    /* while(true) body */
    vec_t while_body_stmts = allocator_create(allocator, &g_vec_type,
                                                &(vec_init_t){.auto_dispose = false});

    /* _result = _iter.next() */
    c_ir_node_t iter_ident = (c_ir_node_t)c_ir_expr_ident_create(
        allocator, "_iter", loc);
    c_ir_node_t next_call_callee = (c_ir_node_t)c_ir_expr_member_create(
        allocator, iter_ident, "next", false, loc);
    vec_t no_args = allocator_create(allocator, &g_vec_type,
                                      &(vec_init_t){.auto_dispose = false});
    c_ir_node_t next_call = (c_ir_node_t)c_ir_expr_call_create(
        allocator, next_call_callee, no_args, loc);
    c_type_t result_type = c_type_primitive(allocator, "void");
    c_ir_node_t result_decl = (c_ir_node_t)c_ir_stmt_local_decl_create(
        allocator, result_type, "_result", next_call, loc);
    vec_push(while_body_stmts, result_decl);

    /* if (_result.done) break */
    c_ir_node_t result_ident = (c_ir_node_t)c_ir_expr_ident_create(
        allocator, "_result", loc);
    c_ir_node_t done_access = (c_ir_node_t)c_ir_expr_member_create(
        allocator, result_ident, "done", false, loc);
    c_ir_node_t break_stmt = (c_ir_node_t)c_ir_stmt_break_create(allocator, loc);
    c_ir_node_t done_if = (c_ir_node_t)c_ir_stmt_if_create(
        allocator, done_access, break_stmt, NULL, loc);
    vec_push(while_body_stmts, done_if);

    /* item = _result.value */
    c_ir_node_t result_ident2 = (c_ir_node_t)c_ir_expr_ident_create(
        allocator, "_result", loc);
    c_ir_node_t value_access = (c_ir_node_t)c_ir_expr_member_create(
        allocator, result_ident2, "value", false, loc);
    c_type_t item_type = c_type_primitive(allocator, "void"); /* placeholder */
    c_ir_node_t item_decl = (c_ir_node_t)c_ir_stmt_local_decl_create(
        allocator, item_type, item_name, value_access, loc);
    vec_push(while_body_stmts, item_decl);

    /* body */
    c_ir_node_t body = lower_stmt(allocator, ctx, n->body, module_hash, defer_stack, unit);
    if (body) vec_push(while_body_stmts, body);

    c_ir_node_t while_body = (c_ir_node_t)c_ir_stmt_block_create(
        allocator, while_body_stmts, loc);

    /* while(true) */
    c_ir_node_t true_cond = (c_ir_node_t)c_ir_expr_bool_create(allocator, true, loc);
    c_ir_node_t while_stmt = (c_ir_node_t)c_ir_stmt_while_create(
        allocator, true_cond, while_body, loc);
    vec_push(outer_stmts, while_stmt);

    return (c_ir_node_t)c_ir_stmt_block_create(allocator, outer_stmts, loc);
  }

  /* ===== Import / Comptime / Test — no C output ===== */
  case CUBEC_NODE_STATEMENT_IMPORT:
  case CUBEC_NODE_STATEMENT_COMPTIME_IF:
  case CUBEC_NODE_STATEMENT_COMPTIME_FOREACH:
  case CUBEC_NODE_STATEMENT_TEST:
  case CUBEC_NODE_STATEMENT_EXPORT_FROM:
  case CUBEC_NODE_STATEMENT_INTERFACE:
  case CUBEC_NODE_STATEMENT_EMPTY:
  case CUBEC_NODE_STATEMENT_DECLARATION_TYPE:
    return NULL;

  default:
    return NULL;
  }
}
