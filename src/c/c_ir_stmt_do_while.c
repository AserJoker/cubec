#include "c/c_ir_stmt_do_while.h"

c_ir_stmt_do_while_t c_ir_stmt_do_while_create(allocator_t allocator,
                                                  c_ir_node_t body,
                                                  c_ir_node_t condition,
                                                  location_t source_loc) {
  c_ir_stmt_do_while_t node = allocator_alloc(allocator, sizeof(struct _c_ir_stmt_do_while_t));
  node->kind = C_IR_STMT_DO_WHILE;
  node->source_loc = source_loc;
  node->body = body;
  node->condition = condition;
  return node;
}

void c_ir_stmt_do_while_dispose(allocator_t allocator, c_ir_stmt_do_while_t *node) {
  if (!node || !*node) return;
  c_ir_stmt_do_while_t n = *node;
  if (n->body) c_ir_dispose(allocator, &n->body);
  if (n->condition) c_ir_dispose(allocator, &n->condition);
  allocator_free(allocator, node);
}
