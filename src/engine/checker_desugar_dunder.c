/**
 * @file checker_desugar_dunder.c
 * @brief Pass 4: Dunder Method Expansion.
 *
 * Strategy:
 *   - Without per-node semantic type info in the desugar phase, we use
 *     generic placeholder function names (__get__, __set__, __dispose__).
 *     The C backend, which has access to the checker's symbol table,
 *     resolves these to the concrete Type__dunder__ functions.
 *   - Subscript (obj[key]): → __get__(&obj, key)
 *   - Subscript assignment (obj[key] = value): → __set__(&obj, key, value)
 *   - using var scope exit: → insert __dispose__(&var) at scope exits
 */
#include "engine/checker_desugar_util.h"
#include "cubec/ast_factory.h"
#include "cubec/statement_expression.h"
#include "cubec/statement_block.h"
#include "cubec/statement_if.h"
#include "cubec/statement_while.h"
#include "cubec/statement_do_while.h"
#include "cubec/statement_for.h"
#include "cubec/statement_foreach.h"
#include "cubec/statement_function.h"
#include "cubec/statement_declaration.h"
#include "cubec/expression_subscript.h"
#include "cubec/expression_assignment.h"
#include "cubec/expression_binary.h"
#include "cubec/declaration_variable.h"
#include "cubec/node.h"

/**
 * @brief Transform subscript expressions into dunder calls.
 */
static node_t _pass4_transform(context_t ctx, node_t expr, void *userdata) {
  (void)userdata;
  if (!expr) return NULL;

  if (expr->kind == CUBEC_NODE_EXPRESSION_SUBSCRIPT) {
    cubec_expression_subscript_t sub = (cubec_expression_subscript_t)expr;
    location_t loc = ((node_t)expr)->location;

    /* Create __get__(&host, index) */
    node_t callee = cubec_ast_create_identifier(ctx, loc, "__get__");
    vec_t args = cubec_ast_create_vec(ctx, false);
    vec_push(args, cubec_ast_create_addr(ctx, loc, sub->host));
    vec_push(args, sub->index);
    return cubec_ast_create_call(ctx, loc, callee, args);
  }

  return NULL;
}

/** @brief Check if an expression is a subscript node. */
static bool _pass4_is_subscript(node_t expr) {
  return expr && expr->kind == CUBEC_NODE_EXPRESSION_SUBSCRIPT;
}

/**
 * @brief Recursively walk expressions + insert using cleanup.
 */
static void _pass4_walk_and_cleanup(context_t ctx, vec_t stmts,
                                     vec_t using_vars) {
  if (!stmts) return;

  for (size_t i = 0; i < vec_get_size(stmts); i++) {
    node_t stmt = (node_t)vec_get(stmts, i);

    /* Walk expressions in this statement (handles SUBSCRIPT → __get__) */
    desugar_walk_stmt_exprs(ctx, stmt, _pass4_transform, NULL);

    /* Also walk ASSIGNMENT expressions for __set__: if left-hand side
     * is a subscript, replace the whole assignment. */
    if (stmt->kind == CUBEC_NODE_STATEMENT_EXPRESSION) {
      cubec_statement_expression_t se =
          (cubec_statement_expression_t)stmt;
      if (se->expression &&
          se->expression->kind == CUBEC_NODE_EXPRESSION_ASSIGNMENT) {
        cubec_expression_binary_t a =
            (cubec_expression_binary_t)se->expression;
        if (_pass4_is_subscript(a->left)) {
          cubec_expression_subscript_t sub =
              (cubec_expression_subscript_t)a->left;
          location_t loc = ((node_t)a)->location;

          /* Create __set__(&host, key, value) */
          node_t callee = cubec_ast_create_identifier(ctx, loc,
                                                        "__set__");
          vec_t args = cubec_ast_create_vec(ctx, false);
          vec_push(args, cubec_ast_create_addr(ctx, loc, sub->host));
          vec_push(args, sub->index);
          vec_push(args, a->right);

          se->expression = cubec_ast_create_call(ctx, loc, callee, args);
        }
      }
    }

    /* Collect using-var declarations from this statement */
    if (stmt->kind == CUBEC_NODE_STATEMENT_DECLARATION) {
      cubec_statement_declaration_t sd =
          (cubec_statement_declaration_t)stmt;
      if (sd->is_using && sd->declarator &&
          sd->declarator->kind == CUBEC_NODE_DECLARATION_VARIABLE) {
        if (!using_vars) using_vars = cubec_ast_create_vec(ctx, false);
        vec_push(using_vars, sd);
      }
    }

    /* Recurse into blocks for nested using vars */
    switch (stmt->kind) {
    case CUBEC_NODE_STATEMENT_BLOCK: {
      cubec_statement_block_t sb = (cubec_statement_block_t)stmt;
      if (sb->statements) {
        vec_t inner_vars = cubec_ast_create_vec(ctx, false);
        _pass4_walk_and_cleanup(ctx, sb->statements, inner_vars);

        /* At end of block, insert __dispose__ for this block's using vars */
        for (size_t j = 0; j < vec_get_size(inner_vars); j++) {
          node_t decl = (node_t)vec_get(inner_vars, j);
          cubec_statement_declaration_t sd =
              (cubec_statement_declaration_t)decl;
          cubec_declaration_variable_t dv =
              (cubec_declaration_variable_t)sd->declarator;
          location_t loc = ((node_t)dv)->location;

          /* Create __dispose__(&var) expression statement */
          node_t callee = cubec_ast_create_identifier(ctx, loc,
                                                        "__dispose__");
          vec_t args = cubec_ast_create_vec(ctx, false);
          vec_push(args, cubec_ast_create_addr(ctx, loc,
                                                dv->identifier));
          node_t call = cubec_ast_create_call(ctx, loc, callee, args);
          node_t expr_stmt = cubec_ast_create_expr_stmt(ctx, loc, call);
          vec_push(sb->statements, expr_stmt);
        }
      }
      break;
    }
    case CUBEC_NODE_STATEMENT_IF: {
      cubec_statement_if_t si = (cubec_statement_if_t)stmt;
      if (si->then_branch &&
          si->then_branch->kind == CUBEC_NODE_STATEMENT_BLOCK) {
        cubec_statement_block_t tb =
            (cubec_statement_block_t)si->then_branch;
        if (tb->statements && using_vars && vec_get_size(using_vars) > 0) {
          vec_t inner = cubec_ast_create_vec(ctx, false);
          /* Inherit parent using_vars for the nested block */
          for (size_t j = 0; j < vec_get_size(using_vars); j++)
            vec_push(inner, vec_get(using_vars, j));
          _pass4_walk_and_cleanup(ctx, tb->statements, inner);
        }
      }
      if (si->else_branch &&
          si->else_branch->kind == CUBEC_NODE_STATEMENT_BLOCK) {
        cubec_statement_block_t eb =
            (cubec_statement_block_t)si->else_branch;
        if (eb->statements && using_vars && vec_get_size(using_vars) > 0) {
          vec_t inner = cubec_ast_create_vec(ctx, false);
          for (size_t j = 0; j < vec_get_size(using_vars); j++)
            vec_push(inner, vec_get(using_vars, j));
          _pass4_walk_and_cleanup(ctx, eb->statements, inner);
        }
      }
      break;
    }
    case CUBEC_NODE_STATEMENT_WHILE:
    case CUBEC_NODE_STATEMENT_DO_WHILE:
    case CUBEC_NODE_STATEMENT_FOR:
    case CUBEC_NODE_STATEMENT_FOREACH:
    case CUBEC_NODE_STATEMENT_FUNCTION: {
      node_t body = NULL;
      if (stmt->kind == CUBEC_NODE_STATEMENT_WHILE)
        body = ((cubec_statement_while_t)stmt)->body;
      else if (stmt->kind == CUBEC_NODE_STATEMENT_DO_WHILE)
        body = ((cubec_statement_do_while_t)stmt)->body;
      else if (stmt->kind == CUBEC_NODE_STATEMENT_FOR)
        body = ((cubec_statement_for_t)stmt)->body;
      else if (stmt->kind == CUBEC_NODE_STATEMENT_FOREACH)
        body = ((cubec_statement_foreach_t)stmt)->body;
      else if (stmt->kind == CUBEC_NODE_STATEMENT_FUNCTION)
        body = ((cubec_statement_function_t)stmt)->body;

      if (body && body->kind == CUBEC_NODE_STATEMENT_BLOCK &&
          using_vars && vec_get_size(using_vars) > 0 &&
          ((cubec_statement_block_t)body)->statements) {
        vec_t inner = cubec_ast_create_vec(ctx, false);
        for (size_t j = 0; j < vec_get_size(using_vars); j++)
          vec_push(inner, vec_get(using_vars, j));
        _pass4_walk_and_cleanup(ctx,
                                 ((cubec_statement_block_t)body)->statements,
                                 inner);
      }
      break;
    }
    default:
      break;
    }
  }
}

void desugar_pass4_dunder(context_t ctx, vec_t statements) {
  _pass4_walk_and_cleanup(ctx, statements, NULL);
}
