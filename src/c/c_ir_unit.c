#include "c/c_ir_unit.h"

c_ir_unit_t c_ir_unit_create(allocator_t allocator, const char *filename,
                              const char *module_hash, location_t source_loc) {
  c_ir_unit_t node = allocator_alloc(allocator, sizeof(struct _c_ir_unit_t));
  node->kind = C_IR_UNIT;
  node->source_loc = source_loc;
  node->filename = allocator_create(allocator, &g_string_type,
                                     &(string_init_t){.str = filename});
  node->module_hash = allocator_create(allocator, &g_string_type,
                                        &(string_init_t){.str = module_hash});
  node->is_library = false;
  node->includes = allocator_create(allocator, &g_vec_type,
                                     &(vec_init_t){.auto_dispose = false});
  node->forward_decls = allocator_create(allocator, &g_vec_type,
                                          &(vec_init_t){.auto_dispose = false});
  node->struct_defs = allocator_create(allocator, &g_vec_type,
                                        &(vec_init_t){.auto_dispose = false});
  node->typedefs = allocator_create(allocator, &g_vec_type,
                                     &(vec_init_t){.auto_dispose = false});
  node->enum_defs = allocator_create(allocator, &g_vec_type,
                                      &(vec_init_t){.auto_dispose = false});
  node->variable_decls = allocator_create(allocator, &g_vec_type,
                                           &(vec_init_t){.auto_dispose = false});
  node->function_decls = allocator_create(allocator, &g_vec_type,
                                           &(vec_init_t){.auto_dispose = false});
  node->function_defs = allocator_create(allocator, &g_vec_type,
                                          &(vec_init_t){.auto_dispose = false});
  node->extern_decls = allocator_create(allocator, &g_vec_type,
                                         &(vec_init_t){.auto_dispose = false});
  return node;
}

void c_ir_unit_dispose(allocator_t allocator, c_ir_unit_t *unit) {
  if (!unit || !*unit) return;
  c_ir_unit_t n = *unit;
  allocator_free(allocator, &n->filename);
  allocator_free(allocator, &n->module_hash);
  c_ir_dispose_vec(allocator, &n->includes);
  c_ir_dispose_vec(allocator, &n->forward_decls);
  c_ir_dispose_vec(allocator, &n->struct_defs);
  c_ir_dispose_vec(allocator, &n->typedefs);
  c_ir_dispose_vec(allocator, &n->enum_defs);
  c_ir_dispose_vec(allocator, &n->variable_decls);
  c_ir_dispose_vec(allocator, &n->function_decls);
  c_ir_dispose_vec(allocator, &n->function_defs);
  c_ir_dispose_vec(allocator, &n->extern_decls);
  allocator_free(allocator, unit);
}
