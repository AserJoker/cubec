#include "c/c_ir_stmt_for.h"

c_ir_stmt_for_t c_ir_stmt_for_create(allocator_t allocator, c_ir_node_t init,
                                        c_ir_node_t condition, c_ir_node_t update,
                                        c_ir_node_t body, location_t source_loc) {
  c_ir_stmt_for_t node = allocator_alloc(allocator, sizeof(struct _c_ir_stmt_for_t));
  node->kind = C_IR_STMT_FOR;
  node->source_loc = source_loc;
  node->init = init;
  node->condition = condition;
  node->update = update;
  node->body = body;
  return node;
}

void c_ir_stmt_for_dispose(allocator_t allocator, c_ir_stmt_for_t *node) {
  if (!node || !*node) return;
  c_ir_stmt_for_t n = *node;
  if (n->init) c_ir_dispose(allocator, &n->init);
  if (n->condition) c_ir_dispose(allocator, &n->condition);
  if (n->update) c_ir_dispose(allocator, &n->update);
  if (n->body) c_ir_dispose(allocator, &n->body);
  allocator_free(allocator, node);
}
