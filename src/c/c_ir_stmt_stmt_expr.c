#include "c/c_ir_stmt_stmt_expr.h"

c_ir_stmt_stmt_expr_t c_ir_stmt_stmt_expr_create(allocator_t allocator, vec_t statements,
                                                    location_t source_loc) {
  c_ir_stmt_stmt_expr_t node = allocator_alloc(allocator, sizeof(struct _c_ir_stmt_stmt_expr_t));
  node->kind = C_IR_STMT_STMT_EXPR;
  node->source_loc = source_loc;
  node->statements = statements;
  return node;
}

void c_ir_stmt_stmt_expr_dispose(allocator_t allocator, c_ir_stmt_stmt_expr_t *node) {
  if (!node || !*node) return;
  c_ir_stmt_stmt_expr_t n = *node;
  c_ir_dispose_vec(allocator, &n->statements);
  allocator_free(allocator, node);
}
