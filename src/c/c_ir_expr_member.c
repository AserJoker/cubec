#include "c/c_ir_expr_member.h"

c_ir_expr_member_t c_ir_expr_member_create(allocator_t allocator, c_ir_node_t object,
                                             const char *field, bool is_arrow,
                                             location_t source_loc) {
  c_ir_expr_member_t node = allocator_alloc(allocator, sizeof(struct _c_ir_expr_member_t));
  node->kind = C_IR_EXPR_MEMBER;
  node->source_loc = source_loc;
  node->object = object;
  node->field = allocator_create(allocator, &g_string_type,
                                  &(string_init_t){.str = field});
  node->is_arrow = is_arrow;
  return node;
}

void c_ir_expr_member_dispose(allocator_t allocator, c_ir_expr_member_t *node) {
  if (!node || !*node) return;
  c_ir_expr_member_t n = *node;
  if (n->object) c_ir_dispose(allocator, &n->object);
  allocator_free(allocator, &n->field);
  allocator_free(allocator, node);
}
