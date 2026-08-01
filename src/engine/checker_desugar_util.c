/**
 * @file checker_desugar_util.c
 * @brief Shared utility functions for the desugar pipeline.
 *
 * Provides expression/statement tree walkers (`desugar_walk_expr`,
 * `desugar_walk_stmt_exprs`), type helpers, and identifier utilities
 * used by all 9 desugar passes.
 */
#include "engine/checker_desugar_util.h"
#include "engine/semantic_type.h"
#include "cubec/statement_block.h"
#include "cubec/statement_function.h"
#include "cubec/statement_struct.h"
#include "cubec/statement_union.h"
#include "cubec/statement_declaration.h"
#include "cubec/statement_expression.h"
#include "cubec/statement_return.h"
#include "cubec/statement_if.h"
#include "cubec/statement_while.h"
#include "cubec/statement_do_while.h"
#include "cubec/statement_for.h"
#include "cubec/statement_foreach.h"
#include "cubec/statement_switch.h"
#include "cubec/statement_defer.h"
#include "cubec/expression_binary.h"
#include "cubec/expression_call.h"
#include "cubec/expression_member.h"
#include "cubec/expression_namespace_access.h"
#include "cubec/expression_assignment.h"
#include "cubec/expression_ternary.h"
#include "cubec/expression_group.h"
#include "cubec/expression_comma.h"
#include "cubec/expression_slice.h"
#include "cubec/expression_spread.h"
#include "cubec/expression_generic_instantiation.h"
#include "cubec/expression_initialize_list.h"
#include "cubec/expression_initialize_field.h"
#include "cubec/expression_typeof.h"
#include "cubec/expression_sizeof.h"
#include "cubec/expression_alignof.h"
#include "cubec/expression_type_qualifier.h"
#include "cubec/expression_type_function.h"
#include "cubec/expression_type_tuple.h"
#include "cubec/expression_postfix_unary.h"
#include "cubec/literal_identifier.h"
#include "cubec/declaration_pointer.h"
#include "cubec/declaration_slice.h"
#include "cubec/declaration_array.h"
#include "cubec/declaration_variable.h"
#include "cubec/struct_field.h"
#include "cubec/union_field.h"
#include "cubec/decorator.h"
#include "cubec/switch_match.h"
#include "cubec/node.h"
#include <string.h>

/* ============================================================================
 *  Identifier + type helpers
 * ============================================================================ */

const char *desugar_ident_str(node_t n) {
  if (!n || n->kind != CUBEC_NODE_LITERAL_IDENTIFIER) return NULL;
  return string_get(((cubec_literal_identifier_t)n)->value);
}

bool desugar_ident_is(node_t n, const char *name) {
  const char *s = desugar_ident_str(n);
  return s && strcmp(s, name) == 0;
}

semantic_type_t desugar_get_semantic_type(context_t ctx, node_t expr) {
  (void)ctx;
  (void)expr;
  /* TODO: return the semantic type stored on the node or computed from context. */
  return NULL;
}

bool desugar_is_tuple_type(semantic_type_t t) {
  return t && semantic_type_get_kind(t) == TYPE_TUPLE;
}

bool desugar_is_slice_type(semantic_type_t t) {
  return t && semantic_type_get_kind(t) == TYPE_SLICE;
}

bool desugar_is_tag_union_type(semantic_type_t t) {
  return t && semantic_type_get_kind(t) == TYPE_UNION;
}

bool desugar_is_function_type(semantic_type_t t) {
  return t && semantic_type_get_kind(t) == TYPE_FUNCTION;
}

bool desugar_is_comptime_stmt(node_t stmt) {
  if (!stmt) return false;
  return stmt->kind == CUBEC_NODE_STATEMENT_COMPTIME_IF ||
         stmt->kind == CUBEC_NODE_STATEMENT_COMPTIME_FOREACH;
}

/* ============================================================================
 *  Expression tree walker
 * ============================================================================ */

/**
 * @brief Recursively walk an expression tree and apply a transformation.
 * @param ctx       Semantic context.
 * @param expr      The expression node (may be replaced).
 * @param transform Transformation function: returns new node or NULL (no change).
 * @param userdata  Opaque user data for the transform callback.
 * @return The (possibly new) expression node.
 */
node_t desugar_walk_expr(context_t ctx, node_t expr,
                         desugar_expr_transform_fn transform, void *userdata) {
  if (!expr) return NULL;

  /* Apply transform first */
  node_t result = transform(ctx, expr, userdata);
  if (result) return result;

  /* Recursively walk children based on node kind */
  switch (expr->kind) {
  case CUBEC_NODE_EXPRESSION_BINARY: {
    cubec_expression_binary_t b = (cubec_expression_binary_t)expr;
    b->left = desugar_walk_expr(ctx, b->left, transform, userdata);
    b->right = desugar_walk_expr(ctx, b->right, transform, userdata);
    return NULL;
  }
  case CUBEC_NODE_EXPRESSION_CALL: {
    cubec_expression_call_t c = (cubec_expression_call_t)expr;
    c->callee = desugar_walk_expr(ctx, c->callee, transform, userdata);
    if (c->arguments) {
      for (size_t i = 0; i < vec_get_size(c->arguments); i++) {
        node_t *arg = (node_t *)vec_get(c->arguments, i);
        *arg = desugar_walk_expr(ctx, *arg, transform, userdata);
      }
    }
    return NULL;
  }
  case CUBEC_NODE_EXPRESSION_MEMBER: {
    cubec_expression_member_t m = (cubec_expression_member_t)expr;
    m->host = desugar_walk_expr(ctx, m->host, transform, userdata);
    return NULL;
  }
  case CUBEC_NODE_EXPRESSION_ASSIGNMENT: {
    /* cubec_expression_assignment_t is typedef of cubec_expression_binary_t,
     * using left=lvalue, right=rvalue. */
    cubec_expression_binary_t a = (cubec_expression_binary_t)expr;
    a->left = desugar_walk_expr(ctx, a->left, transform, userdata);
    a->right = desugar_walk_expr(ctx, a->right, transform, userdata);
    return NULL;
  }
  case CUBEC_NODE_EXPRESSION_TERNARY: {
    cubec_expression_ternary_t t = (cubec_expression_ternary_t)expr;
    t->condition = desugar_walk_expr(ctx, t->condition, transform, userdata);
    t->consequent = desugar_walk_expr(ctx, t->consequent, transform, userdata);
    t->alternate = desugar_walk_expr(ctx, t->alternate, transform, userdata);
    return NULL;
  }
  case CUBEC_NODE_EXPRESSION_GROUP: {
    cubec_expression_group_t g = (cubec_expression_group_t)expr;
    g->inner = desugar_walk_expr(ctx, g->inner, transform, userdata);
    return NULL;
  }
  case CUBEC_NODE_EXPRESSION_COMMA: {
    cubec_expression_comma_t cm = (cubec_expression_comma_t)expr;
    cm->left = desugar_walk_expr(ctx, cm->left, transform, userdata);
    cm->right = desugar_walk_expr(ctx, cm->right, transform, userdata);
    return NULL;
  }
  case CUBEC_NODE_EXPRESSION_GENERIC_INSTANTIATION: {
    cubec_expression_generic_instantiation_t gi =
        (cubec_expression_generic_instantiation_t)expr;
    gi->callee = desugar_walk_expr(ctx, gi->callee, transform, userdata);
    if (gi->arguments) {
      for (size_t i = 0; i < vec_get_size(gi->arguments); i++) {
        node_t *arg = (node_t *)vec_get(gi->arguments, i);
        *arg = desugar_walk_expr(ctx, *arg, transform, userdata);
      }
    }
    return NULL;
  }
  case CUBEC_NODE_EXPRESSION_SLICE: {
    cubec_expression_slice_t s = (cubec_expression_slice_t)expr;
    s->host = desugar_walk_expr(ctx, s->host, transform, userdata);
    s->start = desugar_walk_expr(ctx, s->start, transform, userdata);
    s->length = desugar_walk_expr(ctx, s->length, transform, userdata);
    return NULL;
  }
  case CUBEC_NODE_EXPRESSION_INITIALIZE_LIST: {
    cubec_expression_initialize_list_t il =
        (cubec_expression_initialize_list_t)expr;
    il->type = desugar_walk_expr(ctx, il->type, transform, userdata);
    if (il->items) {
      for (size_t i = 0; i < vec_get_size(il->items); i++) {
        node_t *item = (node_t *)vec_get(il->items, i);
        *item = desugar_walk_expr(ctx, *item, transform, userdata);
      }
    }
    return NULL;
  }
  case CUBEC_NODE_EXPRESSION_INITIALIZE_FIELD: {
    cubec_expression_initialize_field_t f =
        (cubec_expression_initialize_field_t)expr;
    f->value = desugar_walk_expr(ctx, f->value, transform, userdata);
    return NULL;
  }
  case CUBEC_NODE_DECORATOR: {
    cubec_decorator_t d = (cubec_decorator_t)expr;
    d->expression = desugar_walk_expr(ctx, d->expression, transform, userdata);
    return NULL;
  }
  case CUBEC_NODE_EXPRESSION_SPREAD: {
    cubec_expression_spread_t sp = (cubec_expression_spread_t)expr;
    sp->value = desugar_walk_expr(ctx, sp->value, transform, userdata);
    return NULL;
  }
  case CUBEC_NODE_EXPRESSION_NAMESPACE_ACCESS: {
    cubec_expression_namespace_access_t na =
        (cubec_expression_namespace_access_t)expr;
    na->host = desugar_walk_expr(ctx, na->host, transform, userdata);
    return NULL;
  }
  case CUBEC_NODE_EXPRESSION_TYPE_QUALIFIER: {
    cubec_expression_type_qualifier_t tq =
        (cubec_expression_type_qualifier_t)expr;
    tq->type = desugar_walk_expr(ctx, tq->type, transform, userdata);
    return NULL;
  }
  case CUBEC_NODE_EXPRESSION_DEREF:
  case CUBEC_NODE_EXPRESSION_ADDR: {
    /* cubec_expression_postfix_unary_t is typedef of cubec_expression_binary_t,
     * operand is stored in right (left = NULL for prefix). */
    cubec_expression_binary_t pu = (cubec_expression_binary_t)expr;
    pu->right = desugar_walk_expr(ctx, pu->right, transform, userdata);
    return NULL;
  }
  case CUBEC_NODE_EXPRESSION_TYPEOF:
  case CUBEC_NODE_EXPRESSION_SIZEOF:
  case CUBEC_NODE_EXPRESSION_ALIGNOF: {
    /* These share the same pattern: an expression field */
    cubec_expression_typeof_t to = (cubec_expression_typeof_t)expr;
    to->expression = desugar_walk_expr(ctx, to->expression, transform, userdata);
    return NULL;
  }
  /* Declaration nodes used as type expressions */
  case CUBEC_NODE_DECLARATION_POINTER: {
    cubec_declaration_pointer_t dp = (cubec_declaration_pointer_t)expr;
    dp->type = desugar_walk_expr(ctx, dp->type, transform, userdata);
    return NULL;
  }
  case CUBEC_NODE_DECLARATION_ARRAY: {
    cubec_declaration_array_t da = (cubec_declaration_array_t)expr;
    da->type = desugar_walk_expr(ctx, da->type, transform, userdata);
    return NULL;
  }
  case CUBEC_NODE_DECLARATION_SLICE: {
    cubec_declaration_slice_t ds = (cubec_declaration_slice_t)expr;
    ds->type = desugar_walk_expr(ctx, ds->type, transform, userdata);
    return NULL;
  }
  /* Type expression nodes */
  case CUBEC_NODE_EXPRESSION_TYPE_FUNCTION: {
    cubec_expression_type_function_t tf =
        (cubec_expression_type_function_t)expr;
    tf->return_type = desugar_walk_expr(ctx, tf->return_type, transform, userdata);
    if (tf->parameters) {
      for (size_t i = 0; i < vec_get_size(tf->parameters); i++) {
        node_t *p = (node_t *)vec_get(tf->parameters, i);
        *p = desugar_walk_expr(ctx, *p, transform, userdata);
      }
    }
    return NULL;
  }
  case CUBEC_NODE_EXPRESSION_TYPE_TUPLE: {
    cubec_expression_type_tuple_t tt = (cubec_expression_type_tuple_t)expr;
    if (tt->element_types) {
      for (size_t i = 0; i < vec_get_size(tt->element_types); i++) {
        node_t *et = (node_t *)vec_get(tt->element_types, i);
        *et = desugar_walk_expr(ctx, *et, transform, userdata);
      }
    }
    return NULL;
  }
  default:
    /* Leaves (literals, etc.) — no children to walk */
    return NULL;
  }
}

/* ============================================================================
 *  Statement expression walker
 * ============================================================================ */

void desugar_walk_stmt_exprs(context_t ctx, node_t stmt,
                             desugar_expr_transform_fn expr_tx, void *userdata) {
  if (!stmt) return;

  switch (stmt->kind) {
  case CUBEC_NODE_STATEMENT_EXPRESSION: {
    cubec_statement_expression_t se = (cubec_statement_expression_t)stmt;
    se->expression = desugar_walk_expr(ctx, se->expression, expr_tx, userdata);
    break;
  }
  case CUBEC_NODE_STATEMENT_RETURN: {
    cubec_statement_return_t r = (cubec_statement_return_t)stmt;
    r->expression = desugar_walk_expr(ctx, r->expression, expr_tx, userdata);
    break;
  }
  case CUBEC_NODE_STATEMENT_IF: {
    cubec_statement_if_t si = (cubec_statement_if_t)stmt;
    si->condition = desugar_walk_expr(ctx, si->condition, expr_tx, userdata);
    desugar_walk_stmt_exprs(ctx, si->then_branch, expr_tx, userdata);
    desugar_walk_stmt_exprs(ctx, si->else_branch, expr_tx, userdata);
    break;
  }
  case CUBEC_NODE_STATEMENT_WHILE: {
    cubec_statement_while_t sw = (cubec_statement_while_t)stmt;
    sw->condition = desugar_walk_expr(ctx, sw->condition, expr_tx, userdata);
    desugar_walk_stmt_exprs(ctx, sw->body, expr_tx, userdata);
    break;
  }
  case CUBEC_NODE_STATEMENT_DO_WHILE: {
    cubec_statement_do_while_t sdw = (cubec_statement_do_while_t)stmt;
    desugar_walk_stmt_exprs(ctx, sdw->body, expr_tx, userdata);
    sdw->condition = desugar_walk_expr(ctx, sdw->condition, expr_tx, userdata);
    break;
  }
  case CUBEC_NODE_STATEMENT_FOR: {
    cubec_statement_for_t sf = (cubec_statement_for_t)stmt;
    sf->init = desugar_walk_expr(ctx, sf->init, expr_tx, userdata);
    sf->condition = desugar_walk_expr(ctx, sf->condition, expr_tx, userdata);
    sf->increment = desugar_walk_expr(ctx, sf->increment, expr_tx, userdata);
    desugar_walk_stmt_exprs(ctx, sf->body, expr_tx, userdata);
    break;
  }
  case CUBEC_NODE_STATEMENT_FOREACH: {
    cubec_statement_foreach_t sfe = (cubec_statement_foreach_t)stmt;
    sfe->iterator = desugar_walk_expr(ctx, sfe->iterator, expr_tx, userdata);
    sfe->var_type = desugar_walk_expr(ctx, sfe->var_type, expr_tx, userdata);
    desugar_walk_stmt_exprs(ctx, sfe->body, expr_tx, userdata);
    break;
  }
  case CUBEC_NODE_STATEMENT_SWITCH: {
    cubec_statement_switch_t ss = (cubec_statement_switch_t)stmt;
    ss->condition = desugar_walk_expr(ctx, ss->condition, expr_tx, userdata);
    if (ss->matches) {
      for (size_t i = 0; i < vec_get_size(ss->matches); i++) {
        node_t *m = (node_t *)vec_get(ss->matches, i);
        desugar_walk_stmt_exprs(ctx, *m, expr_tx, userdata);
      }
    }
    break;
  }
  case CUBEC_NODE_STATEMENT_BLOCK: {
    cubec_statement_block_t sb = (cubec_statement_block_t)stmt;
    if (sb->statements) {
      for (size_t i = 0; i < vec_get_size(sb->statements); i++) {
        node_t *child = (node_t *)vec_get(sb->statements, i);
        desugar_walk_stmt_exprs(ctx, *child, expr_tx, userdata);
      }
    }
    break;
  }
  case CUBEC_NODE_STATEMENT_DECLARATION: {
    cubec_statement_declaration_t sd = (cubec_statement_declaration_t)stmt;
    /* Walk the declarator expression */
    if (sd->declarator && sd->declarator->kind == CUBEC_NODE_DECLARATION_VARIABLE) {
      cubec_declaration_variable_t dv = (cubec_declaration_variable_t)sd->declarator;
      dv->type = desugar_walk_expr(ctx, dv->type, expr_tx, userdata);
      dv->expression = desugar_walk_expr(ctx, dv->expression, expr_tx, userdata);
    }
    break;
  }
  case CUBEC_NODE_STATEMENT_FUNCTION: {
    cubec_statement_function_t sf = (cubec_statement_function_t)stmt;
    sf->return_type = desugar_walk_expr(ctx, sf->return_type, expr_tx, userdata);
    desugar_walk_stmt_exprs(ctx, sf->body, expr_tx, userdata);
    break;
  }
  case CUBEC_NODE_STATEMENT_STRUCT: {
    cubec_statement_struct_t ss = (cubec_statement_struct_t)stmt;
    if (ss->members) {
      for (size_t i = 0; i < vec_get_size(ss->members); i++) {
        node_t *m = (node_t *)vec_get(ss->members, i);
        desugar_walk_stmt_exprs(ctx, *m, expr_tx, userdata);
      }
    }
    break;
  }
  case CUBEC_NODE_STATEMENT_UNION: {
    cubec_statement_union_t su = (cubec_statement_union_t)stmt;
    if (su->members) {
      for (size_t i = 0; i < vec_get_size(su->members); i++) {
        node_t *m = (node_t *)vec_get(su->members, i);
        desugar_walk_stmt_exprs(ctx, *m, expr_tx, userdata);
      }
    }
    break;
  }
  case CUBEC_NODE_STATEMENT_DEFER: {
    cubec_statement_defer_t sd = (cubec_statement_defer_t)stmt;
    desugar_walk_stmt_exprs(ctx, sd->body, expr_tx, userdata);
    break;
  }
  case CUBEC_NODE_STATEMENT_TEST:
    /* Tests are removed in cleanup pass, but walk in case they survive */
    break;
  default:
    break;
  }
}

void desugar_walk_all_stmt_exprs(context_t ctx, vec_t stmts,
                                 desugar_expr_transform_fn tx, void *userdata) {
  if (!stmts) return;
  for (size_t i = 0; i < vec_get_size(stmts); i++) {
    node_t *stmt = (node_t *)vec_get(stmts, i);
    desugar_walk_stmt_exprs(ctx, *stmt, tx, userdata);
  }
}

void desugar_walk_type_expr(context_t ctx, node_t *type_ptr,
                            desugar_expr_transform_fn tx, void *userdata) {
  if (!type_ptr || !*type_ptr) return;
  *type_ptr = desugar_walk_expr(ctx, *type_ptr, tx, userdata);
}
