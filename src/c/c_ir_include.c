#include "c/c_ir_include.h"

c_ir_include_t c_ir_include_create(allocator_t allocator, const char *path,
                                     bool is_system, location_t source_loc) {
  c_ir_include_t node = allocator_alloc(allocator, sizeof(struct _c_ir_include_t));
  node->kind = C_IR_INCLUDE;
  node->source_loc = source_loc;
  node->path = allocator_create(allocator, &g_string_type,
                                 &(string_init_t){.str = path});
  node->is_system = is_system;
  return node;
}

void c_ir_include_dispose(allocator_t allocator, c_ir_include_t *node) {
  if (!node || !*node) return;
  c_ir_include_t n = *node;
  allocator_free(allocator, &n->path);
  allocator_free(allocator, node);
}
