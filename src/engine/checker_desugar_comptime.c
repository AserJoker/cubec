/**
 * @file checker_desugar_comptime.c
 * @brief Pass 2: Comptime Elimination.
 *
 * Strategy:
 *   - COMPTIME_IF: Evaluate condition via comptime_eval_expr(), replace
 *     with the taken branch's statements (unwrapping from block).
 *   - COMPTIME_FOREACH: Execute via comptime_eval_exec_comptime_foreach(),
 *     which evaluates the iterator and runs body side effects. The node
 *     is removed after execution — side effects persist in comptime env.
 *   - typeof/sizeof/alignof: Kept in AST. The checker already computed
 *     their types; the C backend handles them directly.
 */
#include "engine/checker_desugar_util.h"
#include "engine/comptime_eval.h"
#include "cubec/statement_block.h"
#include "cubec/statement_if.h"
#include "cubec/statement_while.h"
#include "cubec/statement_do_while.h"
#include "cubec/statement_for.h"
#include "cubec/statement_foreach.h"
#include "cubec/statement_switch.h"
#include "cubec/statement_function.h"
#include "cubec/statement_defer.h"
#include "cubec/statement_comptime.h"
#include "cubec/switch_match.h"
#include "cubec/node.h"

/** @brief Recursively eliminate comptime constructs from a statement vec.
 *  Modifies `stmts` in-place. */
static void _pass2_eliminate_vec(context_t ctx, vec_t stmts) {
  if (!stmts) return;

  comptime_eval_t eval = ctx->comptime_eval;
  if (!eval) return; /* Can't eliminate comptime without evaluator */

  for (size_t i = 0; i < vec_get_size(stmts); ) {
    node_t stmt = (node_t)vec_get(stmts, i);

    /* ---- Recursively descend into statement children ---- */
    switch (stmt->kind) {
    case CUBEC_NODE_STATEMENT_BLOCK: {
      cubec_statement_block_t sb = (cubec_statement_block_t)stmt;
      _pass2_eliminate_vec(ctx, sb->statements);
      i++;
      continue;
    }
    case CUBEC_NODE_STATEMENT_IF: {
      cubec_statement_if_t si = (cubec_statement_if_t)stmt;
      if (si->then_branch && si->then_branch->kind == CUBEC_NODE_STATEMENT_BLOCK)
        _pass2_eliminate_vec(
            ctx, ((cubec_statement_block_t)si->then_branch)->statements);
      if (si->else_branch && si->else_branch->kind == CUBEC_NODE_STATEMENT_BLOCK)
        _pass2_eliminate_vec(
            ctx, ((cubec_statement_block_t)si->else_branch)->statements);
      i++;
      continue;
    }
    case CUBEC_NODE_STATEMENT_WHILE: {
      cubec_statement_while_t sw = (cubec_statement_while_t)stmt;
      if (sw->body && sw->body->kind == CUBEC_NODE_STATEMENT_BLOCK)
        _pass2_eliminate_vec(
            ctx, ((cubec_statement_block_t)sw->body)->statements);
      i++;
      continue;
    }
    case CUBEC_NODE_STATEMENT_DO_WHILE: {
      cubec_statement_do_while_t sdw = (cubec_statement_do_while_t)stmt;
      if (sdw->body && sdw->body->kind == CUBEC_NODE_STATEMENT_BLOCK)
        _pass2_eliminate_vec(
            ctx, ((cubec_statement_block_t)sdw->body)->statements);
      i++;
      continue;
    }
    case CUBEC_NODE_STATEMENT_FOR: {
      cubec_statement_for_t sf = (cubec_statement_for_t)stmt;
      if (sf->body && sf->body->kind == CUBEC_NODE_STATEMENT_BLOCK)
        _pass2_eliminate_vec(
            ctx, ((cubec_statement_block_t)sf->body)->statements);
      i++;
      continue;
    }
    case CUBEC_NODE_STATEMENT_FOREACH: {
      cubec_statement_foreach_t sfe = (cubec_statement_foreach_t)stmt;
      if (sfe->body && sfe->body->kind == CUBEC_NODE_STATEMENT_BLOCK)
        _pass2_eliminate_vec(
            ctx, ((cubec_statement_block_t)sfe->body)->statements);
      i++;
      continue;
    }
    case CUBEC_NODE_STATEMENT_SWITCH: {
      cubec_statement_switch_t ss = (cubec_statement_switch_t)stmt;
      if (ss->matches) {
        _pass2_eliminate_vec(ctx, ss->matches);
      }
      i++;
      continue;
    }
    case CUBEC_NODE_STATEMENT_FUNCTION: {
      cubec_statement_function_t sf = (cubec_statement_function_t)stmt;
      if (sf->body && sf->body->kind == CUBEC_NODE_STATEMENT_BLOCK)
        _pass2_eliminate_vec(
            ctx, ((cubec_statement_block_t)sf->body)->statements);
      i++;
      continue;
    }
    case CUBEC_NODE_STATEMENT_DEFER: {
      cubec_statement_defer_t sd = (cubec_statement_defer_t)stmt;
      if (sd->body && sd->body->kind == CUBEC_NODE_STATEMENT_BLOCK)
        _pass2_eliminate_vec(
            ctx, ((cubec_statement_block_t)sd->body)->statements);
      i++;
      continue;
    }
    case CUBEC_NODE_SWITCH_MATCH: {
      cubec_switch_match_t sm = (cubec_switch_match_t)stmt;
      if (sm->body && sm->body->kind == CUBEC_NODE_STATEMENT_BLOCK)
        _pass2_eliminate_vec(
            ctx, ((cubec_statement_block_t)sm->body)->statements);
      i++;
      continue;
    }

    /* ---- COMPTIME_IF elimination ---- */
    case CUBEC_NODE_STATEMENT_COMPTIME_IF: {
      cubec_statement_comptime_if_t ci =
          (cubec_statement_comptime_if_t)stmt;

      /* Evaluate condition at compile time */
      comptime_value_t cond_val =
          comptime_eval_expr(eval, ctx, ci->condition);
      bool truthy = comptime_value_is_truthy(cond_val);
      node_t taken = truthy ? ci->then_branch : ci->else_branch;

      /* Remove the COMPTIME_IF node */
      vec_remove(stmts, i);

      if (taken) {
        /* Unwrap block and splice its statements in place */
        if (taken->kind == CUBEC_NODE_STATEMENT_BLOCK) {
          cubec_statement_block_t tb = (cubec_statement_block_t)taken;
          vec_t block_stmts = tb->statements;
          if (block_stmts) {
            size_t count = vec_get_size(block_stmts);
            for (size_t j = 0; j < count; j++) {
              vec_insert(stmts, i + j, vec_get(block_stmts, j));
            }
            i += count;
            /* Recursively process inserted statements */
            _pass2_eliminate_vec(ctx, stmts);
          }
          continue;
        } else {
          /* Single statement (shouldn't normally happen, but handle it) */
          vec_insert(stmts, i, taken);
          i++;
          continue;
        }
      }
      /* Neither branch taken — COMPTIME_IF removed, next stmt slides up */
      continue;
    }

    /* ---- COMPTIME_FOREACH elimination ---- */
    case CUBEC_NODE_STATEMENT_COMPTIME_FOREACH: {
      /* Execute the foreach at compile time. Side effects (comptime
       * var bindings, comptime function calls) persist in the comptime
       * environment. The loop body is executed for each iteration. */
      (void)comptime_eval_exec_comptime_foreach(eval, ctx, stmt);

      /* Remove the COMPTIME_FOREACH from the AST */
      vec_remove(stmts, i);
      /* Next statement slides down; i stays the same */
      continue;
    }

    default:
      i++;
      continue;
    }
  }
}

void desugar_pass2_comptime(context_t ctx, vec_t statements) {
  _pass2_eliminate_vec(ctx, statements);
}
