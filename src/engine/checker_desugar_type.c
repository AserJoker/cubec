/**
 * @file checker_desugar_type.c
 * @brief Pass 3: Type Degradation — tuple/slice/tag-union → struct.
 *
 * Strategy:
 *   - Tuple <T1, T2, ...> → struct __tuple_N { T1 _0; T2 _1; ... }
 *   - Slice []T → struct __slice_N { T* ptr; u64 start; u64 length; }
 *   - Tag union → struct { u64 __tag; cunion { ... } __data; }
 *
 * This pass:
 *   1. Walks all type expression positions, replacing tuples/slices
 *      with identifiers referencing generated struct declarations.
 *   2. Generates unique struct declarations and appends them to global scope.
 *   3. Transforms tag union statements into struct + cunion pairs.
 */
#include "engine/checker_desugar_util.h"
#include "cubec/ast_factory.h"
#include "cubec/expression_type_tuple.h"
#include "cubec/declaration_slice.h"
#include "cubec/statement_function.h"
#include "cubec/statement_struct.h"
#include "cubec/statement_union.h"
#include "cubec/statement_declaration.h"
#include "cubec/statement_foreach.h"
#include "cubec/statement_switch.h"
#include "cubec/struct_field.h"
#include "cubec/union_field.h"
#include "cubec/declaration_variable.h"
#include "cubec/literal_identifier.h"
#include "cubec/function_argument.h"
#include "cubec/switch_match.h"
#include "cubec/node.h"
#include <stdio.h>
#include <string.h>

/** @brief State for Pass 3 type degradation. */
typedef struct {
  vec_t  global_stmts;   /**< Global statements vec (to append structs). */
  size_t counter;         /**< Monotonic counter for unique struct names. */
} _pass3_state_t;

/** @brief Create a u64 type expression (identifier "u64"). */
static node_t _pass3_u64_type(context_t ctx, location_t loc) {
  return cubec_ast_create_identifier(ctx, loc, "u64");
}

/** @brief Generate struct declaration for a slice type. */
static node_t _pass3_gen_slice_struct(context_t ctx, location_t loc,
                                       node_t element_type,
                                       _pass3_state_t *state) {
  char name_buf[64];
  snprintf(name_buf, sizeof(name_buf), "__slice_%zu", state->counter++);

  /* Create pointer-to-element type: T* */
  node_t ptr_type = cubec_ast_create_pointer_type(ctx, loc,
                                                   element_type, false, false);

  /* Create struct members */
  vec_t members = cubec_ast_create_vec(ctx, true);
  vec_push(members, cubec_ast_create_struct_field(ctx, loc, "ptr",
                                                    ptr_type, false));
  vec_push(members, cubec_ast_create_struct_field(ctx, loc, "start",
                                                    _pass3_u64_type(ctx, loc),
                                                    false));
  vec_push(members, cubec_ast_create_struct_field(ctx, loc, "length",
                                                    _pass3_u64_type(ctx, loc),
                                                    false));

  /* Create struct statement */
  node_t struct_decl = cubec_ast_create_struct_stmt(ctx, loc, name_buf,
                                                      members, false, NULL);
  vec_push(state->global_stmts, struct_decl);

  return cubec_ast_create_identifier(ctx, loc, name_buf);
}

/** @brief Generate struct declaration for a tuple type. */
static node_t _pass3_gen_tuple_struct(context_t ctx, location_t loc,
                                       vec_t element_types,
                                       _pass3_state_t *state) {
  char name_buf[64];
  snprintf(name_buf, sizeof(name_buf), "__tuple_%zu", state->counter++);

  /* Create struct members: _0, _1, ... */
  vec_t members = cubec_ast_create_vec(ctx, true);
  size_t n = vec_get_size(element_types);
  for (size_t i = 0; i < n; i++) {
    char field_buf[16];
    snprintf(field_buf, sizeof(field_buf), "_%zu", i);
    node_t elem_type = (node_t)vec_get(element_types, i);
    vec_push(members, cubec_ast_create_struct_field(ctx, loc, field_buf,
                                                      elem_type, false));
  }

  /* Create struct statement */
  node_t struct_decl = cubec_ast_create_struct_stmt(ctx, loc, name_buf,
                                                      members, false, NULL);
  vec_push(state->global_stmts, struct_decl);

  return cubec_ast_create_identifier(ctx, loc, name_buf);
}

/** @brief Transform callback: replaces tuple/slice type expressions with
 *  identifiers referencing generated structs. */
static node_t _pass3_type_transform(context_t ctx, node_t expr,
                                     void *userdata) {
  _pass3_state_t *state = (_pass3_state_t *)userdata;
  if (!expr) return NULL;

  location_t loc = ((node_t)expr)->location;

  if (expr->kind == CUBEC_NODE_EXPRESSION_TYPE_TUPLE) {
    cubec_expression_type_tuple_t tt = (cubec_expression_type_tuple_t)expr;
    return _pass3_gen_tuple_struct(ctx, loc, tt->element_types, state);
  }

  if (expr->kind == CUBEC_NODE_DECLARATION_SLICE) {
    cubec_declaration_slice_t ds = (cubec_declaration_slice_t)expr;
    return _pass3_gen_slice_struct(ctx, loc, ds->type, state);
  }

  return NULL;
}

/**
 * @brief Recursively walk ALL type expression positions in statements,
 *        applying the Pass 3 type transform.
 */
static void _pass3_walk_stmt_types(context_t ctx, node_t stmt,
                                    _pass3_state_t *state) {
  if (!stmt) return;

  /* Walk expression positions via desugar_walk_expr with transform */
  desugar_walk_stmt_exprs(ctx, stmt, _pass3_type_transform, state);

  /* Walk additional type annotation positions that desugar_walk_stmt_exprs
   * might not reach (the same comprehensive walk as in Pass 1). */
  switch (stmt->kind) {
  case CUBEC_NODE_STATEMENT_FUNCTION: {
    cubec_statement_function_t sf = (cubec_statement_function_t)stmt;
    desugar_walk_type_expr(ctx, &sf->return_type, _pass3_type_transform, state);
    if (sf->arguments) {
      for (size_t j = 0; j < vec_get_size(sf->arguments); j++) {
        cubec_function_argument_t fa =
            (cubec_function_argument_t)vec_get(sf->arguments, j);
        desugar_walk_type_expr(ctx, &fa->type, _pass3_type_transform, state);
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
          desugar_walk_type_expr(ctx, &sfld->type, _pass3_type_transform, state);
        } else if (m->kind == CUBEC_NODE_STATEMENT_DECLARATION) {
          _pass3_walk_stmt_types(ctx, m, state);
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
      desugar_walk_type_expr(ctx, &dv->type, _pass3_type_transform, state);
      desugar_walk_type_expr(ctx, &dv->expression, _pass3_type_transform, state);
    }
    break;
  }
  case CUBEC_NODE_STATEMENT_UNION:
    /* Tag unions are handled below; skip their field types during walk */
    break;
  case CUBEC_NODE_STATEMENT_FOREACH: {
    cubec_statement_foreach_t sfe =
        (cubec_statement_foreach_t)stmt;
    desugar_walk_type_expr(ctx, &sfe->var_type, _pass3_type_transform, state);
    desugar_walk_type_expr(ctx, &sfe->iterator, _pass3_type_transform, state);
    break;
  }
  case CUBEC_NODE_STATEMENT_SWITCH: {
    cubec_statement_switch_t ss = (cubec_statement_switch_t)stmt;
    desugar_walk_type_expr(ctx, &ss->condition, _pass3_type_transform, state);
    if (ss->matches) {
      for (size_t j = 0; j < vec_get_size(ss->matches); j++) {
        node_t m = (node_t)vec_get(ss->matches, j);
        _pass3_walk_stmt_types(ctx, m, state);
      }
    }
    break;
  }
  default:
    break;
  }
}

/** @brief Check if a union field type is void (should be skipped). */
static bool _pass3_is_void_type(node_t type) {
  if (!type) return true;
  if (type->kind == CUBEC_NODE_LITERAL_IDENTIFIER) {
    const char *name = string_get(((cubec_literal_identifier_t)type)->value);
    return name && strcmp(name, "void") == 0;
  }
  return false;
}

/**
 * @brief Transform a tag union statement into struct + cunion.
 */
static void _pass3_degrade_tag_union(context_t ctx,
                                      cubec_statement_union_t su,
                                      _pass3_state_t *state) {
  location_t loc = ((node_t)su)->location;
  const char *union_name = desugar_ident_str(su->name);
  if (!union_name) union_name = "anon";

  /* Build cunion variant fields (skip void-typed fields) */
  vec_t variant_fields = cubec_ast_create_vec(ctx, true);
  if (su->members) {
    for (size_t i = 0; i < vec_get_size(su->members); i++) {
      node_t m = (node_t)vec_get(su->members, i);
      if (m->kind != CUBEC_NODE_UNION_FIELD) continue;
      cubec_union_field_t uf = (cubec_union_field_t)m;
      if (_pass3_is_void_type(uf->type)) continue;

      const char *fname = desugar_ident_str(uf->name);
      if (!fname) fname = "anon";
      vec_push(variant_fields,
               cubec_ast_create_struct_field(ctx, loc, fname,
                                              uf->type, false));
    }
  }

  /* Generate cunion name */
  char cunion_name[64];
  snprintf(cunion_name, sizeof(cunion_name), "__variant_%s", union_name);

  /* Create cunion statement */
  node_t cunion_decl = cubec_ast_create_cunion_stmt(ctx, loc,
                                                      cunion_name,
                                                      variant_fields);
  vec_push(state->global_stmts, cunion_decl);

  /* Create the replacement struct: struct Name { u64 __tag; cunion_name __data; } */
  vec_t struct_members = cubec_ast_create_vec(ctx, true);
  vec_push(struct_members,
           cubec_ast_create_struct_field(ctx, loc, "__tag",
                                          _pass3_u64_type(ctx, loc), false));

  /* __data field: type is the cunion name */
  node_t data_type = cubec_ast_create_identifier(ctx, loc, cunion_name);
  vec_push(struct_members,
           cubec_ast_create_struct_field(ctx, loc, "__data",
                                          data_type, false));

  /* Create struct statement (keeping original union's export status) */
  node_t struct_decl = cubec_ast_create_struct_stmt(ctx, loc, union_name,
                                                      struct_members,
                                                      su->is_export, NULL);
  vec_push(state->global_stmts, struct_decl);
}

void desugar_pass3_type_degrade(context_t ctx, vec_t statements) {
  if (!statements) return;

  _pass3_state_t state = {statements, 0};

  /* Phase A: Walk all type expression positions and degrade
   * tuples + slices in-place. New structs are appended to `statements`. */
  for (size_t i = 0; i < vec_get_size(statements); i++) {
    node_t stmt = (node_t)vec_get(statements, i);
    _pass3_walk_stmt_types(ctx, stmt, &state);
  }

  /* Phase B: Transform tag unions.
   * We iterate in reverse because we replace union statements in-place. */
  for (size_t i = 0; i < vec_get_size(statements); ) {
    node_t stmt = (node_t)vec_get(statements, i);
    if (stmt->kind == CUBEC_NODE_STATEMENT_UNION) {
      cubec_statement_union_t su = (cubec_statement_union_t)stmt;
      _pass3_degrade_tag_union(ctx, su, &state);
      /* Remove original union statement */
      vec_remove(statements, i);
      /* Continue at same index (next stmt slides down) */
      continue;
    }
    i++;
  }
}
