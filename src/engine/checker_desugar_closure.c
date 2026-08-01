/**
 * @file checker_desugar_closure.c
 * @brief Pass 5: Closure Lowering — closures → {func_ptr, env_ptr} fat pointers.
 *
 * Strategy:
 *   - Find CUBEC_NODE_EXPRESSION_FUNCTION nodes (anonymous function expressions).
 *   - Lift each to a global function declaration with `void* __env` as the
 *     first parameter.
 *   - Replace the expression_function with:
 *     * No captures → `&__closure_N` (function pointer)
 *     * With captures → fat pointer struct init (future enhancement)
 */
#include "engine/checker_desugar_util.h"
#include "cubec/statement_expression.h"
#include "cubec/statement_block.h"
#include "cubec/statement_if.h"
#include "cubec/statement_while.h"
#include "cubec/statement_do_while.h"
#include "cubec/statement_for.h"
#include "cubec/statement_foreach.h"
#include "cubec/statement_switch.h"
#include "cubec/statement_function.h"
#include "cubec/expression_function.h"
#include "cubec/function_argument.h"
#include "cubec/function_capture.h"
#include "cubec/switch_match.h"
#include "cubec/node.h"
#include "cubec/declaration_pointer.h"
#include "cubec/expression_postfix_unary.h"
#include "cubec/literal_identifier.h"
#include <stdio.h>

void desugar_pass5_closure(context_t ctx, vec_t statements) {
  if (!statements) return;

  size_t closure_counter = 0;

  for (size_t i = 0; i < vec_get_size(statements); i++) {
    node_t stmt = (node_t)vec_get(statements, i);

    /* Walk expressions in this statement looking for anonymous functions */
    if (stmt->kind == CUBEC_NODE_STATEMENT_EXPRESSION) {
      cubec_statement_expression_t se =
          (cubec_statement_expression_t)stmt;
      node_t expr = se->expression;

      if (expr && expr->kind == CUBEC_NODE_EXPRESSION_FUNCTION) {
        cubec_expression_function_t ef =
            (cubec_expression_function_t)expr;
        location_t loc = ((node_t)ef)->location;

        /* Generate unique closure name */
        char name_buf[64];
        snprintf(name_buf, sizeof(name_buf), "__closure_%zu",
                 closure_counter++);

        /* Build new argument list with void* __env as first param */
        vec_t new_args = cubec_ast_create_vec(ctx, true);
        vec_push(new_args, cubec_ast_create_func_arg(ctx, loc,
                                                       "__env",
                                                       cubec_ast_create_pointer_type(
                                                           ctx, loc,
                                                           cubec_ast_create_identifier(ctx, loc, "void"),
                                                           false, false)));

        /* Copy original arguments */
        if (ef->arguments) {
          for (size_t j = 0; j < vec_get_size(ef->arguments); j++) {
            vec_push(new_args, vec_get(ef->arguments, j));
          }
        }

        /* Create lifted global function */
        node_t lifted_fn = cubec_ast_create_func_stmt(
            ctx, loc, name_buf, new_args, ef->return_type,
            ef->body, false /* not export */, false /* not inline */,
            false /* not extern */, false /* not builtin */,
            false /* not comptime */, ef->is_c_variadic);

        /* Add to global statements */
        vec_push(statements, lifted_fn);

        /* Replace closure expression with function pointer */
        node_t fn_ident = cubec_ast_create_identifier(ctx, loc,
                                                         name_buf);
        se->expression = cubec_ast_create_addr(ctx, loc, fn_ident);
      }
    }

    /* Recurse into nested blocks for more closure expressions */
    switch (stmt->kind) {
    case CUBEC_NODE_STATEMENT_BLOCK: {
      cubec_statement_block_t sb = (cubec_statement_block_t)stmt;
      desugar_pass5_closure(ctx, sb->statements);
      break;
    }
    case CUBEC_NODE_STATEMENT_IF: {
      cubec_statement_if_t si = (cubec_statement_if_t)stmt;
      if (si->then_branch &&
          si->then_branch->kind == CUBEC_NODE_STATEMENT_BLOCK)
        desugar_pass5_closure(
            ctx, ((cubec_statement_block_t)si->then_branch)->statements);
      if (si->else_branch &&
          si->else_branch->kind == CUBEC_NODE_STATEMENT_BLOCK)
        desugar_pass5_closure(
            ctx, ((cubec_statement_block_t)si->else_branch)->statements);
      break;
    }
    case CUBEC_NODE_STATEMENT_WHILE:
    case CUBEC_NODE_STATEMENT_DO_WHILE:
    case CUBEC_NODE_STATEMENT_FOR:
    case CUBEC_NODE_STATEMENT_FOREACH: {
      node_t body = NULL;
      if (stmt->kind == CUBEC_NODE_STATEMENT_WHILE)
        body = ((cubec_statement_while_t)stmt)->body;
      else if (stmt->kind == CUBEC_NODE_STATEMENT_DO_WHILE)
        body = ((cubec_statement_do_while_t)stmt)->body;
      else if (stmt->kind == CUBEC_NODE_STATEMENT_FOR)
        body = ((cubec_statement_for_t)stmt)->body;
      else
        body = ((cubec_statement_foreach_t)stmt)->body;

      if (body && body->kind == CUBEC_NODE_STATEMENT_BLOCK)
        desugar_pass5_closure(
            ctx, ((cubec_statement_block_t)body)->statements);
      break;
    }
    case CUBEC_NODE_STATEMENT_FUNCTION: {
      cubec_statement_function_t sf =
          (cubec_statement_function_t)stmt;
      if (sf->body && sf->body->kind == CUBEC_NODE_STATEMENT_BLOCK)
        desugar_pass5_closure(
            ctx, ((cubec_statement_block_t)sf->body)->statements);
      break;
    }
    case CUBEC_NODE_STATEMENT_SWITCH: {
      cubec_statement_switch_t ss =
          (cubec_statement_switch_t)stmt;
      if (ss->matches)
        desugar_pass5_closure(ctx, ss->matches);
      break;
    }
    case CUBEC_NODE_SWITCH_MATCH: {
      cubec_switch_match_t sm = (cubec_switch_match_t)stmt;
      if (sm->body && sm->body->kind == CUBEC_NODE_STATEMENT_BLOCK)
        desugar_pass5_closure(
            ctx, ((cubec_statement_block_t)sm->body)->statements);
      break;
    }
    default:
      break;
    }
  }
}
