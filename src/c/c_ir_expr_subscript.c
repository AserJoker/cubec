#include "c/c_ir_expr_subscript.h"

c_ir_expr_subscript_t c_ir_expr_subscript_create(allocator_t allocator,
                                                    c_ir_node_t object,
                                                    c_ir_node_t index,
                                                    location_t source_loc) {
  c_ir_expr_subscript_t node = allocator_alloc(allocator, sizeof(struct _c_ir_expr_subscript_t));
  node->kind = C_IR_EXPR_SUBSCRIPT;
  node->source_loc = source_loc;
  node->object = object;
  node->index = index;
  return node;
}

void c_ir_expr_subscript_dispose(allocator_t allocator, c_ir_expr_subscript_t *node) {
  if (!node || !*node) return;
  c_ir_expr_subscript_t n = *node;
  if (n->object) c_ir_dispose(allocator, &n->object);
  if (n->index) c_ir_dispose(allocator, &n->index);
  allocator_free(allocator, node);
}
