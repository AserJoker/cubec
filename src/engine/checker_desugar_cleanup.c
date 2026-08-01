/**
 * @file checker_desugar_cleanup.c
 * @brief Pass 9: Cleanup + Validation — remove meta-nodes, validate output subset.
 *
 * Strategy:
 *   - Remove C-superfluous nodes: decorators, interfaces, imports,
 *     tests, and undefined literals.
 *   - Validate that ALL remaining nodes belong to the simplified subset.
 */
#include "engine/checker_desugar_util.h"
#include "cubec/ast_factory.h"
#include "cubec/statement_block.h"
#include "cubec/statement_if.h"
#include "cubec/statement_while.h"
#include "cubec/statement_do_while.h"
#include "cubec/statement_for.h"
#include "cubec/statement_foreach.h"
#include "cubec/statement_switch.h"
#include "cubec/statement_function.h"
#include "cubec/statement_defer.h"
#include "cubec/statement_declaration.h"
#include "cubec/statement_expression.h"
#include "cubec/statement_struct.h"
#include "cubec/literal_undefined.h"
#include "cubec/switch_match.h"
#include "cubec/node.h"
#include <stdio.h>

/** @brief Check if a node kind is valid for the simplified C output. */
static bool _is_valid_simplified_node(uint32_t kind) {
  switch (kind) {
  /* Global declarations */
  case CUBEC_NODE_STATEMENT_STRUCT:
  case CUBEC_NODE_STATEMENT_ENUM:
  case CUBEC_NODE_STATEMENT_CUNION:
  case CUBEC_NODE_STATEMENT_FUNCTION:
  case CUBEC_NODE_STATEMENT_DECLARATION:
  /* Control flow */
  case CUBEC_NODE_STATEMENT_IF:
  case CUBEC_NODE_STATEMENT_WHILE:
  case CUBEC_NODE_STATEMENT_DO_WHILE:
  case CUBEC_NODE_STATEMENT_FOR:
  case CUBEC_NODE_STATEMENT_FOREACH:
  case CUBEC_NODE_STATEMENT_SWITCH:
  case CUBEC_NODE_STATEMENT_DEFER:
  /* Jump */
  case CUBEC_NODE_STATEMENT_RETURN:
  case CUBEC_NODE_STATEMENT_BREAK:
  case CUBEC_NODE_STATEMENT_CONTINUE:
  /* Block */
  case CUBEC_NODE_STATEMENT_BLOCK:
  case CUBEC_NODE_STATEMENT_EXPRESSION:
  case CUBEC_NODE_STATEMENT_EMPTY:
  /* Expressions */
  case CUBEC_NODE_EXPRESSION_CALL:
  case CUBEC_NODE_EXPRESSION_BINARY:
  case CUBEC_NODE_EXPRESSION_ASSIGNMENT:
  case CUBEC_NODE_EXPRESSION_MEMBER:
  case CUBEC_NODE_EXPRESSION_SUBSCRIPT:
  case CUBEC_NODE_EXPRESSION_DEREF:
  case CUBEC_NODE_EXPRESSION_ADDR:
  case CUBEC_NODE_EXPRESSION_TERNARY:
  case CUBEC_NODE_EXPRESSION_GROUP:
  case CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS:
  case CUBEC_NODE_EXPRESSION_COMMA:
  /* Literals */
  case CUBEC_NODE_LITERAL_CHAR:
  case CUBEC_NODE_LITERAL_NUMERIC:
  case CUBEC_NODE_LITERAL_STRING:
  case CUBEC_NODE_LITERAL_IDENTIFIER:
  /* Type expressions */
  case CUBEC_NODE_EXPRESSION_TYPE_STRUCT:
  case CUBEC_NODE_EXPRESSION_TYPE_ENUM:
  case CUBEC_NODE_EXPRESSION_TYPE_UNION:
  case CUBEC_NODE_EXPRESSION_TYPE_FUNCTION:
  case CUBEC_NODE_DECLARATION_POINTER:
  case CUBEC_NODE_DECLARATION_ARRAY:
  case CUBEC_NODE_DECLARATION_SLICE:
  case CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER:
  /* Initialize */
  case CUBEC_NODE_EXPRESSION_INITIALIZE_LIST:
  case CUBEC_NODE_EXPRESSION_INITIALIZE_FIELD:
  /* Sub-structures */
  case CUBEC_NODE_STRUCT_FIELD:
  case CUBEC_NODE_UNION_FIELD:
  case CUBEC_NODE_ENUM_ITEM:
  case CUBEC_NODE_FUNCTION_ARGUMENT:
  case CUBEC_NODE_SWITCH_MATCH:
  case CUBEC_NODE_DECLARATION_VARIABLE:
  /* Program itself */
  case CUBEC_NODE_PROGRAM:
    return true;
  default:
    return false;
  }
}

/**
 * @brief Recursively remove invalid nodes and validate the tree.
 * @return Number of removed nodes.
 */
static size_t _pass9_cleanup_vec(context_t ctx, vec_t stmts) {
  if (!stmts) return 0;

  size_t removed = 0;

  for (size_t i = 0; i < vec_get_size(stmts); ) {
    node_t stmt = (node_t)vec_get(stmts, i);

    /* Remove decorator nodes */
    if (stmt->kind == CUBEC_NODE_DECORATOR) {
      vec_remove(stmts, i);
      removed++;
      continue;
    }

    /* Remove interface declarations */
    if (stmt->kind == CUBEC_NODE_STATEMENT_INTERFACE) {
      vec_remove(stmts, i);
      removed++;
      continue;
    }

    /* Remove import statements */
    if (stmt->kind == CUBEC_NODE_STATEMENT_IMPORT) {
      vec_remove(stmts, i);
      removed++;
      continue;
    }

    /* Remove test blocks */
    if (stmt->kind == CUBEC_NODE_STATEMENT_TEST) {
      vec_remove(stmts, i);
      removed++;
      continue;
    }

    /* Replace undefined literal with zero-initialization */
    if (stmt->kind == CUBEC_NODE_STATEMENT_EXPRESSION) {
      cubec_statement_expression_t se =
          (cubec_statement_expression_t)stmt;
      if (se->expression &&
          se->expression->kind == CUBEC_NODE_LITERAL_UNDEFINED) {
        /* Replace `undefined` with `.{0}` (zero-initialize) */
        location_t loc = ((node_t)se->expression)->location;
        vec_t items = cubec_ast_create_vec(ctx, false);
        se->expression = cubec_ast_create_initialize_list(
            ctx, loc, NULL /* anonymous type */, items, false);
      }
    }

    /* Validate this node */
    if (!_is_valid_simplified_node(stmt->kind)) {
      fprintf(stderr, "Pass 9 validation: unexpected node kind %u\n",
              (unsigned)stmt->kind);
    }

    /* Recurse into block children */
    switch (stmt->kind) {
    case CUBEC_NODE_STATEMENT_BLOCK:
      removed += _pass9_cleanup_vec(
          ctx, ((cubec_statement_block_t)stmt)->statements);
      break;
    case CUBEC_NODE_STATEMENT_IF: {
      cubec_statement_if_t si = (cubec_statement_if_t)stmt;
      if (si->then_branch &&
          si->then_branch->kind == CUBEC_NODE_STATEMENT_BLOCK)
        removed += _pass9_cleanup_vec(
            ctx, ((cubec_statement_block_t)si->then_branch)->statements);
      if (si->else_branch &&
          si->else_branch->kind == CUBEC_NODE_STATEMENT_BLOCK)
        removed += _pass9_cleanup_vec(
            ctx, ((cubec_statement_block_t)si->else_branch)->statements);
      break;
    }
    case CUBEC_NODE_STATEMENT_WHILE:
      if (((cubec_statement_while_t)stmt)->body &&
          ((cubec_statement_while_t)stmt)->body->kind ==
              CUBEC_NODE_STATEMENT_BLOCK)
        removed += _pass9_cleanup_vec(
            ctx, ((cubec_statement_block_t)((cubec_statement_while_t)stmt)
                      ->body)
                     ->statements);
      break;
    case CUBEC_NODE_STATEMENT_DO_WHILE:
      if (((cubec_statement_do_while_t)stmt)->body &&
          ((cubec_statement_do_while_t)stmt)->body->kind ==
              CUBEC_NODE_STATEMENT_BLOCK)
        removed += _pass9_cleanup_vec(
            ctx,
            ((cubec_statement_block_t)((cubec_statement_do_while_t)stmt)->body)
                ->statements);
      break;
    case CUBEC_NODE_STATEMENT_FOR:
      if (((cubec_statement_for_t)stmt)->body &&
          ((cubec_statement_for_t)stmt)->body->kind ==
              CUBEC_NODE_STATEMENT_BLOCK)
        removed += _pass9_cleanup_vec(
            ctx,
            ((cubec_statement_block_t)((cubec_statement_for_t)stmt)->body)
                ->statements);
      break;
    case CUBEC_NODE_STATEMENT_FOREACH:
      if (((cubec_statement_foreach_t)stmt)->body &&
          ((cubec_statement_foreach_t)stmt)->body->kind ==
              CUBEC_NODE_STATEMENT_BLOCK)
        removed += _pass9_cleanup_vec(
            ctx,
            ((cubec_statement_block_t)((cubec_statement_foreach_t)stmt)->body)
                ->statements);
      break;
    case CUBEC_NODE_STATEMENT_FUNCTION:
      if (((cubec_statement_function_t)stmt)->body &&
          ((cubec_statement_function_t)stmt)->body->kind ==
              CUBEC_NODE_STATEMENT_BLOCK)
        removed += _pass9_cleanup_vec(
            ctx,
            ((cubec_statement_block_t)((cubec_statement_function_t)stmt)->body)
                ->statements);
      break;
    case CUBEC_NODE_STATEMENT_SWITCH:
      if (((cubec_statement_switch_t)stmt)->matches)
        removed += _pass9_cleanup_vec(
            ctx, ((cubec_statement_switch_t)stmt)->matches);
      break;
    case CUBEC_NODE_SWITCH_MATCH: {
      cubec_switch_match_t sm = (cubec_switch_match_t)stmt;
      if (sm->body && sm->body->kind == CUBEC_NODE_STATEMENT_BLOCK)
        removed += _pass9_cleanup_vec(
            ctx, ((cubec_statement_block_t)sm->body)->statements);
      break;
    }
    case CUBEC_NODE_STATEMENT_DEFER:
      if (((cubec_statement_defer_t)stmt)->body &&
          ((cubec_statement_defer_t)stmt)->body->kind ==
              CUBEC_NODE_STATEMENT_BLOCK)
        removed += _pass9_cleanup_vec(
            ctx,
            ((cubec_statement_block_t)((cubec_statement_defer_t)stmt)->body)
                ->statements);
      break;
    default:
      break;
    }

    /* Clean decorator vecs from declaration and function nodes */
    if (stmt->kind == CUBEC_NODE_STATEMENT_DECLARATION) {
      cubec_statement_declaration_t sd =
          (cubec_statement_declaration_t)stmt;
      sd->decorators = NULL; /* Drop decorators */
    }
    if (stmt->kind == CUBEC_NODE_STATEMENT_FUNCTION) {
      cubec_statement_function_t sf =
          (cubec_statement_function_t)stmt;
      sf->decorators = NULL;
    }
    if (stmt->kind == CUBEC_NODE_STATEMENT_STRUCT) {
      ((cubec_statement_struct_t)stmt)->decorators = NULL;
    }

    i++;
  }

  return removed;
}

void desugar_pass9_cleanup(context_t ctx, vec_t statements) {
  size_t removed = _pass9_cleanup_vec(ctx, statements);
  if (removed > 0) {
    fprintf(stdout, "Pass 9: removed %zu non-C nodes\n", (unsigned)removed);
  }
}
