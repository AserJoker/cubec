#include "c/c_ir_stmt_if.h"

c_ir_stmt_if_t c_ir_stmt_if_create(allocator_t allocator, c_ir_node_t condition,
                                     c_ir_node_t then_branch, c_ir_node_t else_branch,
                                     location_t source_loc) {
  c_ir_stmt_if_t node = allocator_alloc(allocator, sizeof(struct _c_ir_stmt_if_t));
  node->kind = C_IR_STMT_IF;
  node->source_loc = source_loc;
  node->condition = condition;
  node->then_branch = then_branch;
  node->else_branch = else_branch;
  return node;
}

void c_ir_stmt_if_dispose(allocator_t allocator, c_ir_stmt_if_t *node) {
  if (!node || !*node) return;
  c_ir_stmt_if_t n = *node;
  if (n->condition) c_ir_dispose(allocator, &n->condition);
  if (n->then_branch) c_ir_dispose(allocator, &n->then_branch);
  if (n->else_branch) c_ir_dispose(allocator, &n->else_branch);
  allocator_free(allocator, node);
}
