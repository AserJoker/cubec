#include "c/c_ir_expr_call.h"

c_ir_expr_call_t c_ir_expr_call_create(allocator_t allocator, c_ir_node_t callee,
                                          vec_t arguments, location_t source_loc) {
  c_ir_expr_call_t node = allocator_alloc(allocator, sizeof(struct _c_ir_expr_call_t));
  node->kind = C_IR_EXPR_CALL;
  node->source_loc = source_loc;
  node->callee = callee;
  node->arguments = arguments;
  return node;
}

void c_ir_expr_call_dispose(allocator_t allocator, c_ir_expr_call_t *node) {
  if (!node || !*node) return;
  c_ir_expr_call_t n = *node;
  if (n->callee) c_ir_dispose(allocator, &n->callee);
  c_ir_dispose_vec(allocator, &n->arguments);
  allocator_free(allocator, node);
}
