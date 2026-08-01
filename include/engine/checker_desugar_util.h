#ifndef _H_CUBEC_ENGINE_CHECKER_DESUGAR_UTIL_
#define _H_CUBEC_ENGINE_CHECKER_DESUGAR_UTIL_
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 *  Shared desugar utility functions — declared here, defined in
 *  checker_desugar_util.c.  Each pass file includes this header.
 * ============================================================================ */

/** @brief Extract identifier string from an identifier literal node. */
const char *desugar_ident_str(node_t n);
bool        desugar_ident_is(node_t n, const char *name);

/** @brief Semantic type helpers (stub until type info is wired into AST). */
semantic_type_t desugar_get_semantic_type(context_t ctx, node_t expr);
bool            desugar_is_tuple_type(semantic_type_t t);
bool            desugar_is_slice_type(semantic_type_t t);
bool            desugar_is_tag_union_type(semantic_type_t t);
bool            desugar_is_function_type(semantic_type_t t);
bool            desugar_is_comptime_stmt(node_t stmt);

/* ---- Expression / statement tree walker ---- */

/**
 * @brief Expression transform callback.
 * @param ctx      Semantic context.
 * @param expr     The expression node (may be replaced).
 * @param userdata Opaque user data.
 * @return Replacement node, or NULL to keep the original and recurse.
 */
typedef node_t (*desugar_expr_transform_fn)(context_t ctx, node_t expr,
                                            void *userdata);

/** @brief Recursively walk an expression tree. */
node_t desugar_walk_expr(context_t ctx, node_t expr,
                         desugar_expr_transform_fn transform, void *userdata);

/** @brief Walk a single statement's expression children. */
void desugar_walk_stmt_exprs(context_t ctx, node_t stmt,
                             desugar_expr_transform_fn expr_tx,
                             void *userdata);

/** @brief Walk all statements in a vec. */
void desugar_walk_all_stmt_exprs(context_t ctx, vec_t stmts,
                                 desugar_expr_transform_fn tx,
                                 void *userdata);

/** @brief Walk a type-expression pointer through transform. */
void desugar_walk_type_expr(context_t ctx, node_t *type_ptr,
                            desugar_expr_transform_fn tx, void *userdata);

/* ---- 9-pass desugar pipeline ---- */

void desugar_pass1_mono(context_t ctx, vec_t statements);
void desugar_pass2_comptime(context_t ctx, vec_t statements);
void desugar_pass3_type_degrade(context_t ctx, vec_t statements);
void desugar_pass4_dunder(context_t ctx, vec_t statements);
void desugar_pass5_closure(context_t ctx, vec_t statements);
void desugar_pass6_hoist(context_t ctx, vec_t statements);
void desugar_pass7_stmt_rewrite(context_t ctx, vec_t statements);
void desugar_pass8_ptr_arith(context_t ctx, vec_t statements);
void desugar_pass9_cleanup(context_t ctx, vec_t statements);

#ifdef __cplusplus
}
#endif
#endif
