#include "c/c_ir_expr_cast.h"

c_ir_expr_cast_t c_ir_expr_cast_create(allocator_t allocator, c_type_t type,
                                          c_ir_node_t operand, location_t source_loc) {
  c_ir_expr_cast_t node = allocator_alloc(allocator, sizeof(struct _c_ir_expr_cast_t));
  node->kind = C_IR_EXPR_CAST;
  node->source_loc = source_loc;
  node->type = type;
  node->operand = operand;
  return node;
}

void c_ir_expr_cast_dispose(allocator_t allocator, c_ir_expr_cast_t *node) {
  if (!node || !*node) return;
  c_ir_expr_cast_t n = *node;
  c_type_dispose(allocator, &n->type);
  if (n->operand) c_ir_dispose(allocator, &n->operand);
  allocator_free(allocator, node);
}
