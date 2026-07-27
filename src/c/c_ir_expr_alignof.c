#include "c/c_ir_expr_alignof.h"

c_ir_expr_alignof_t c_ir_expr_alignof_create(allocator_t allocator, c_type_t type,
                                                location_t source_loc) {
  c_ir_expr_alignof_t node = allocator_alloc(allocator, sizeof(struct _c_ir_expr_alignof_t));
  node->kind = C_IR_EXPR_ALIGNOF;
  node->source_loc = source_loc;
  node->type = type;
  return node;
}

void c_ir_expr_alignof_dispose(allocator_t allocator, c_ir_expr_alignof_t *node) {
  if (!node || !*node) return;
  c_ir_expr_alignof_t n = *node;
  c_type_dispose(allocator, &n->type);
  allocator_free(allocator, node);
}
