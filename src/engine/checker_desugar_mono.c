/**
 * @file checker_desugar_mono.c
 * @brief Pass 1: Monomorphization — generic instances → concrete copies.
 *
 * Strategy:
 *   - The semantic checker already resolved all generic instantiations and
 *     created concrete function/type instances (stored in the symbol table).
 *   - This pass strips the GENERIC_INSTANTIATION syntax sugar from the AST:
 *     foo[T]  →  foo   (the semantic type on 'foo' already points to the
 *                        correct instantiated symbol)
 *   - Generic function declarations (templates) with generic_params are
 *     preserved for now — the C backend can skip them if they have 0
 *     instantiations.
 *   - We walk: expressions, type annotations in declarations, function
 *     parameter types, struct field types, etc.
 */
#include "engine/checker_desugar_util.h"
#include "cubec/statement_function.h"
#include "cubec/statement_struct.h"
#include "cubec/statement_union.h"
#include "cubec/statement_declaration.h"
#include "cubec/statement_foreach.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/function_argument.h"
#include "cubec/struct_field.h"
#include "cubec/union_field.h"
#include "cubec/declaration_variable.h"
#include "cubec/node.h"

/** @brief Transform: unwrap GENERIC_INSTANTIATION → callee. */
static node_t _pass1_transform(context_t ctx, node_t expr, void *userdata) {
  (void)ctx;
  (void)userdata;
  if (!expr) return NULL;
  if (expr->kind == CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION) {
    cubec_expression_generic_instantiation_t gi =
        (cubec_expression_generic_instantiation_t)expr;
    return gi->callee;
  }
  return NULL;
}

void desugar_pass1_mono(context_t ctx, vec_t statements) {
  if (!statements) return;

  /* Phase A: Walk every statement's expressions + type annotations */
  for (size_t i = 0; i < vec_get_size(statements); i++) {
    node_t stmt = *(node_t *)vec_get(statements, i);
    if (!stmt) continue;

    /* Standard expression walker handles CALL callee, MEMBER host, etc. */
    desugar_walk_stmt_exprs(ctx, stmt, _pass1_transform, NULL);

    /* ALSO walk type annotations that desugar_walk_stmt_exprs might miss */
    switch (stmt->kind) {
    case CUBEC_NODE_STATEMENT_FUNCTION: {
      cubec_statement_function_t sf = (cubec_statement_function_t)stmt;
      desugar_walk_type_expr(ctx, &sf->return_type, _pass1_transform, NULL);
      if (sf->arguments) {
        for (size_t j = 0; j < vec_get_size(sf->arguments); j++) {
          cubec_function_argument_t fa =
              (cubec_function_argument_t)vec_get(sf->arguments, j);
          desugar_walk_type_expr(ctx, &fa->type, _pass1_transform, NULL);
        }
      }
      break;
    }
    case CUBEC_NODE_STATEMENT_STRUCT: {
      cubec_statement_struct_t ss = (cubec_statement_struct_t)stmt;
      if (ss->members) {
        for (size_t j = 0; j < vec_get_size(ss->members); j++) {
          node_t m = (node_t)vec_get(ss->members, j);
          if (m->kind == CUBEC_NODE_STRUCT_FIELD) {
            cubec_struct_field_t sfld = (cubec_struct_field_t)m;
            desugar_walk_type_expr(ctx, &sfld->type, _pass1_transform, NULL);
          } else if (m->kind == CUBEC_NODE_STATEMENT_DECLARATION) {
            cubec_statement_declaration_t sd =
                (cubec_statement_declaration_t)m;
            if (sd->declarator &&
                sd->declarator->kind == CUBEC_NODE_DECLARATION_VARIABLE) {
              cubec_declaration_variable_t dv =
                  (cubec_declaration_variable_t)sd->declarator;
              desugar_walk_type_expr(ctx, &dv->type, _pass1_transform, NULL);
              desugar_walk_type_expr(ctx, &dv->expression, _pass1_transform, NULL);
            }
          }
        }
      }
      break;
    }
    case CUBEC_NODE_STATEMENT_UNION: {
      cubec_statement_union_t su = (cubec_statement_union_t)stmt;
      if (su->members) {
        for (size_t j = 0; j < vec_get_size(su->members); j++) {
          node_t m = (node_t)vec_get(su->members, j);
          if (m->kind == CUBEC_NODE_UNION_FIELD) {
            cubec_union_field_t uf = (cubec_union_field_t)m;
            desugar_walk_type_expr(ctx, &uf->type, _pass1_transform, NULL);
          }
        }
      }
      break;
    }
    case CUBEC_NODE_STATEMENT_DECLARATION: {
      cubec_statement_declaration_t sd =
          (cubec_statement_declaration_t)stmt;
      if (sd->declarator &&
          sd->declarator->kind == CUBEC_NODE_DECLARATION_VARIABLE) {
        cubec_declaration_variable_t dv =
            (cubec_declaration_variable_t)sd->declarator;
        desugar_walk_type_expr(ctx, &dv->type, _pass1_transform, NULL);
        desugar_walk_type_expr(ctx, &dv->expression, _pass1_transform, NULL);
      }
      break;
    }
    case CUBEC_NODE_STATEMENT_FOREACH: {
      cubec_statement_foreach_t sfe =
          (cubec_statement_foreach_t)stmt;
      desugar_walk_type_expr(ctx, &sfe->var_type, _pass1_transform, NULL);
      desugar_walk_type_expr(ctx, &sfe->iterator, _pass1_transform, NULL);
      break;
    }
    default:
      break;
    }
  }
}
