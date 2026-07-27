#include "c/c_ir_stmt_local_decl.h"

c_ir_stmt_local_decl_t c_ir_stmt_local_decl_create(allocator_t allocator, c_type_t type,
                                                      const char *name, c_ir_node_t init,
                                                      location_t source_loc) {
  c_ir_stmt_local_decl_t node = allocator_alloc(allocator, sizeof(struct _c_ir_stmt_local_decl_t));
  node->kind = C_IR_STMT_LOCAL_DECL;
  node->source_loc = source_loc;
  node->type = type;
  node->name = allocator_create(allocator, &g_string_type,
                                 &(string_init_t){.str = name});
  node->init = init;
  return node;
}

void c_ir_stmt_local_decl_dispose(allocator_t allocator, c_ir_stmt_local_decl_t *node) {
  if (!node || !*node) return;
  c_ir_stmt_local_decl_t n = *node;
  c_type_dispose(allocator, &n->type);
  allocator_free(allocator, &n->name);
  if (n->init) c_ir_dispose(allocator, &n->init);
  allocator_free(allocator, node);
}
