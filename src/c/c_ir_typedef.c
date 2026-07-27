#include "c/c_ir_typedef.h"

c_ir_typedef_t c_ir_typedef_create(allocator_t allocator, c_type_t type,
                                     const char *name, location_t source_loc) {
  c_ir_typedef_t node = allocator_alloc(allocator, sizeof(struct _c_ir_typedef_t));
  node->kind = C_IR_TYPEDEF;
  node->source_loc = source_loc;
  node->type = type;
  node->name = allocator_create(allocator, &g_string_type,
                                 &(string_init_t){.str = name});
  return node;
}

void c_ir_typedef_dispose(allocator_t allocator, c_ir_typedef_t *node) {
  if (!node || !*node) return;
  c_ir_typedef_t n = *node;
  c_type_dispose(allocator, &n->type);
  allocator_free(allocator, &n->name);
  allocator_free(allocator, node);
}
