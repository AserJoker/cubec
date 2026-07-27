#include "c/c_ir_expr_binary.h"

c_ir_expr_binary_t c_ir_expr_binary_create(allocator_t allocator, const char *op,
                                              c_ir_node_t left, c_ir_node_t right,
                                              location_t source_loc) {
  c_ir_expr_binary_t node = allocator_alloc(allocator, sizeof(struct _c_ir_expr_binary_t));
  node->kind = C_IR_EXPR_BINARY;
  node->source_loc = source_loc;
  node->op = allocator_create(allocator, &g_string_type,
                               &(string_init_t){.str = op});
  node->left = left;
  node->right = right;
  return node;
}

void c_ir_expr_binary_dispose(allocator_t allocator, c_ir_expr_binary_t *node) {
  if (!node || !*node) return;
  c_ir_expr_binary_t n = *node;
  allocator_free(allocator, &n->op);
  if (n->left) c_ir_dispose(allocator, &n->left);
  if (n->right) c_ir_dispose(allocator, &n->right);
  allocator_free(allocator, node);
}
