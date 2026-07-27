#include "c/c_ir_expr_compound.h"

c_ir_expr_compound_t c_ir_expr_compound_create(allocator_t allocator, c_type_t type,
                                                  vec_t fields, location_t source_loc) {
  c_ir_expr_compound_t node = allocator_alloc(allocator, sizeof(struct _c_ir_expr_compound_t));
  node->kind = C_IR_EXPR_COMPOUND;
  node->source_loc = source_loc;
  node->type = type;
  node->fields = fields;
  return node;
}

void c_ir_expr_compound_dispose(allocator_t allocator, c_ir_expr_compound_t *node) {
  if (!node || !*node) return;
  c_ir_expr_compound_t n = *node;
  c_type_dispose(allocator, &n->type);
  c_ir_dispose_vec(allocator, &n->fields);
  allocator_free(allocator, node);
}
