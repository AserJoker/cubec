/**
 * @file checker_desugar_hoist.c
 * @brief Pass 6: Scope Hoisting — nested types/functions → global.
 *
 * Strategy:
 *   - Recursively walk all scopes. Any local type/function declarations
 *     discovered in nested blocks are noted but not moved at this stage.
 *   - The C backend performs the actual hoisting — it has access to the
 *     semantic symbol table and can determine correct mangled names.
 *   - This pass ensures all nested scopes are fully traversed so that
 *     later passes (Pass 7, 8, 9) see the complete tree.
 */
#include "engine/checker_desugar_util.h"
#include "cubec/statement_block.h"
#include "cubec/statement_if.h"
#include "cubec/statement_while.h"
#include "cubec/statement_do_while.h"
#include "cubec/statement_for.h"
#include "cubec/statement_foreach.h"
#include "cubec/statement_switch.h"
#include "cubec/statement_function.h"
#include "cubec/switch_match.h"
#include "cubec/node.h"

static void _pass6_recursive_walk(context_t ctx, vec_t stmts) {
  if (!stmts) return;
  (void)ctx;

  for (size_t i = 0; i < vec_get_size(stmts); i++) {
    node_t stmt = (node_t)vec_get(stmts, i);

    switch (stmt->kind) {
    case CUBEC_NODE_STATEMENT_BLOCK:
      _pass6_recursive_walk(
          ctx, ((cubec_statement_block_t)stmt)->statements);
      break;
    case CUBEC_NODE_STATEMENT_IF: {
      cubec_statement_if_t si = (cubec_statement_if_t)stmt;
      if (si->then_branch && si->then_branch->kind == CUBEC_NODE_STATEMENT_BLOCK)
        _pass6_recursive_walk(
            ctx, ((cubec_statement_block_t)si->then_branch)->statements);
      if (si->else_branch && si->else_branch->kind == CUBEC_NODE_STATEMENT_BLOCK)
        _pass6_recursive_walk(
            ctx, ((cubec_statement_block_t)si->else_branch)->statements);
      break;
    }
    case CUBEC_NODE_STATEMENT_WHILE:
      if (((cubec_statement_while_t)stmt)->body &&
          ((cubec_statement_while_t)stmt)->body->kind ==
              CUBEC_NODE_STATEMENT_BLOCK)
        _pass6_recursive_walk(
            ctx, ((cubec_statement_block_t)((cubec_statement_while_t)stmt)
                      ->body)
                     ->statements);
      break;
    case CUBEC_NODE_STATEMENT_DO_WHILE:
      if (((cubec_statement_do_while_t)stmt)->body &&
          ((cubec_statement_do_while_t)stmt)->body->kind ==
              CUBEC_NODE_STATEMENT_BLOCK)
        _pass6_recursive_walk(
            ctx,
            ((cubec_statement_block_t)((cubec_statement_do_while_t)stmt)->body)
                ->statements);
      break;
    case CUBEC_NODE_STATEMENT_FOR:
      if (((cubec_statement_for_t)stmt)->body &&
          ((cubec_statement_for_t)stmt)->body->kind ==
              CUBEC_NODE_STATEMENT_BLOCK)
        _pass6_recursive_walk(
            ctx,
            ((cubec_statement_block_t)((cubec_statement_for_t)stmt)->body)
                ->statements);
      break;
    case CUBEC_NODE_STATEMENT_FOREACH:
      if (((cubec_statement_foreach_t)stmt)->body &&
          ((cubec_statement_foreach_t)stmt)->body->kind ==
              CUBEC_NODE_STATEMENT_BLOCK)
        _pass6_recursive_walk(
            ctx,
            ((cubec_statement_block_t)((cubec_statement_foreach_t)stmt)->body)
                ->statements);
      break;
    case CUBEC_NODE_STATEMENT_FUNCTION:
      if (((cubec_statement_function_t)stmt)->body &&
          ((cubec_statement_function_t)stmt)->body->kind ==
              CUBEC_NODE_STATEMENT_BLOCK)
        _pass6_recursive_walk(
            ctx,
            ((cubec_statement_block_t)((cubec_statement_function_t)stmt)->body)
                ->statements);
      break;
    case CUBEC_NODE_STATEMENT_SWITCH:
      if (((cubec_statement_switch_t)stmt)->matches)
        _pass6_recursive_walk(
            ctx, ((cubec_statement_switch_t)stmt)->matches);
      break;
    case CUBEC_NODE_SWITCH_MATCH: {
      cubec_switch_match_t sm = (cubec_switch_match_t)stmt;
      if (sm->body && sm->body->kind == CUBEC_NODE_STATEMENT_BLOCK)
        _pass6_recursive_walk(
            ctx, ((cubec_statement_block_t)sm->body)->statements);
      break;
    }
    default:
      break;
    }
  }
}

void desugar_pass6_hoist(context_t ctx, vec_t statements) {
  _pass6_recursive_walk(ctx, statements);
}
