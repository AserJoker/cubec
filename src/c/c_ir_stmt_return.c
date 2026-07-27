#include "c/c_ir_stmt_return.h"

c_ir_stmt_return_t c_ir_stmt_return_create(allocator_t allocator, c_ir_node_t value,
                                              location_t source_loc) {
  c_ir_stmt_return_t node = allocator_alloc(allocator, sizeof(struct _c_ir_stmt_return_t));
  node->kind = C_IR_STMT_RETURN;
  node->source_loc = source_loc;
  node->value = value;
  return node;
}

void c_ir_stmt_return_dispose(allocator_t allocator, c_ir_stmt_return_t *node) {
  if (!node || !*node) return;
  c_ir_stmt_return_t n = *node;
  if (n->value) c_ir_dispose(allocator, &n->value);
  allocator_free(allocator, node);
}
