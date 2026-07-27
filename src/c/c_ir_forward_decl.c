#include "c/c_ir_forward_decl.h"

c_ir_forward_decl_t c_ir_forward_decl_create(allocator_t allocator,
                                                const char *name,
                                                location_t source_loc) {
  c_ir_forward_decl_t node = allocator_alloc(allocator, sizeof(struct _c_ir_forward_decl_t));
  node->kind = C_IR_FORWARD_DECL;
  node->source_loc = source_loc;
  node->name = allocator_create(allocator, &g_string_type,
                                 &(string_init_t){.str = name});
  return node;
}

void c_ir_forward_decl_dispose(allocator_t allocator, c_ir_forward_decl_t *node) {
  if (!node || !*node) return;
  c_ir_forward_decl_t n = *node;
  allocator_free(allocator, &n->name);
  allocator_free(allocator, node);
}
