#include "c/c_ir_expr_unary.h"

c_ir_expr_unary_t c_ir_expr_unary_create(allocator_t allocator, const char *op,
                                            c_ir_node_t operand, bool is_prefix,
                                            location_t source_loc) {
  c_ir_expr_unary_t node = allocator_alloc(allocator, sizeof(struct _c_ir_expr_unary_t));
  node->kind = C_IR_EXPR_UNARY;
  node->source_loc = source_loc;
  node->op = allocator_create(allocator, &g_string_type,
                               &(string_init_t){.str = op});
  node->operand = operand;
  node->is_prefix = is_prefix;
  return node;
}

void c_ir_expr_unary_dispose(allocator_t allocator, c_ir_expr_unary_t *node) {
  if (!node || !*node) return;
  c_ir_expr_unary_t n = *node;
  allocator_free(allocator, &n->op);
  if (n->operand) c_ir_dispose(allocator, &n->operand);
  allocator_free(allocator, node);
}
