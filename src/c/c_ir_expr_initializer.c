#include "c/c_ir_expr_initializer.h"

c_ir_expr_initializer_t c_ir_expr_initializer_create(allocator_t allocator,
                                                        const char *name,
                                                        c_ir_node_t value,
                                                        location_t source_loc) {
  c_ir_expr_initializer_t node = allocator_alloc(allocator, sizeof(struct _c_ir_expr_initializer_t));
  node->kind = C_IR_EXPR_INITIALIZER;
  node->source_loc = source_loc;
  node->name = allocator_create(allocator, &g_string_type,
                                 &(string_init_t){.str = name});
  node->index = 0;
  node->value = value;
  node->is_designated = true;
  return node;
}

c_ir_expr_initializer_t c_ir_expr_initializer_create_indexed(allocator_t allocator,
                                                               size_t index,
                                                               c_ir_node_t value,
                                                               location_t source_loc) {
  c_ir_expr_initializer_t node = allocator_alloc(allocator, sizeof(struct _c_ir_expr_initializer_t));
  node->kind = C_IR_EXPR_INITIALIZER;
  node->source_loc = source_loc;
  node->name = NULL;
  node->index = index;
  node->value = value;
  node->is_designated = false;
  return node;
}

void c_ir_expr_initializer_dispose(allocator_t allocator, c_ir_expr_initializer_t *node) {
  if (!node || !*node) return;
  c_ir_expr_initializer_t n = *node;
  if (n->name) allocator_free(allocator, &n->name);
  if (n->value) c_ir_dispose(allocator, &n->value);
  allocator_free(allocator, node);
}
