#include "c/c_ir_expr_ternary.h"

c_ir_expr_ternary_t c_ir_expr_ternary_create(allocator_t allocator,
                                                c_ir_node_t condition,
                                                c_ir_node_t consequent,
                                                c_ir_node_t alternate,
                                                location_t source_loc) {
  c_ir_expr_ternary_t node = allocator_alloc(allocator, sizeof(struct _c_ir_expr_ternary_t));
  node->kind = C_IR_EXPR_TERNARY;
  node->source_loc = source_loc;
  node->condition = condition;
  node->consequent = consequent;
  node->alternate = alternate;
  return node;
}

void c_ir_expr_ternary_dispose(allocator_t allocator, c_ir_expr_ternary_t *node) {
  if (!node || !*node) return;
  c_ir_expr_ternary_t n = *node;
  if (n->condition) c_ir_dispose(allocator, &n->condition);
  if (n->consequent) c_ir_dispose(allocator, &n->consequent);
  if (n->alternate) c_ir_dispose(allocator, &n->alternate);
  allocator_free(allocator, node);
}
