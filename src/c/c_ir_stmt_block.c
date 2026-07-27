#include "c/c_ir_stmt_block.h"

c_ir_stmt_block_t c_ir_stmt_block_create(allocator_t allocator, vec_t statements,
                                            location_t source_loc) {
  c_ir_stmt_block_t node = allocator_alloc(allocator, sizeof(struct _c_ir_stmt_block_t));
  node->kind = C_IR_STMT_BLOCK;
  node->source_loc = source_loc;
  node->statements = statements;
  return node;
}

void c_ir_stmt_block_dispose(allocator_t allocator, c_ir_stmt_block_t *node) {
  if (!node || !*node) return;
  c_ir_stmt_block_t n = *node;
  c_ir_dispose_vec(allocator, &n->statements);
  allocator_free(allocator, node);
}
