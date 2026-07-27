#include "c/c_ir_stmt_expr.h"

c_ir_stmt_expr_t c_ir_stmt_expr_create(allocator_t allocator, c_ir_node_t expression,
                                          location_t source_loc) {
  c_ir_stmt_expr_t node = allocator_alloc(allocator, sizeof(struct _c_ir_stmt_expr_t));
  node->kind = C_IR_STMT_EXPR;
  node->source_loc = source_loc;
  node->expression = expression;
  return node;
}

void c_ir_stmt_expr_dispose(allocator_t allocator, c_ir_stmt_expr_t *node) {
  if (!node || !*node) return;
  c_ir_stmt_expr_t n = *node;
  if (n->expression) c_ir_dispose(allocator, &n->expression);
  allocator_free(allocator, node);
}
