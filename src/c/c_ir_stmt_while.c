#include "c/c_ir_stmt_while.h"

c_ir_stmt_while_t c_ir_stmt_while_create(allocator_t allocator, c_ir_node_t condition,
                                            c_ir_node_t body, location_t source_loc) {
  c_ir_stmt_while_t node = allocator_alloc(allocator, sizeof(struct _c_ir_stmt_while_t));
  node->kind = C_IR_STMT_WHILE;
  node->source_loc = source_loc;
  node->condition = condition;
  node->body = body;
  return node;
}

void c_ir_stmt_while_dispose(allocator_t allocator, c_ir_stmt_while_t *node) {
  if (!node || !*node) return;
  c_ir_stmt_while_t n = *node;
  if (n->condition) c_ir_dispose(allocator, &n->condition);
  if (n->body) c_ir_dispose(allocator, &n->body);
  allocator_free(allocator, node);
}
