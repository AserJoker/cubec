#include "c/c_ir_function.h"

c_ir_param_t c_ir_param_create(allocator_t allocator, c_type_t type,
                                 const char *name) {
  c_ir_param_t param = allocator_alloc(allocator, sizeof(struct _c_ir_param_t));
  param->type = type;
  param->name = allocator_create(allocator, &g_string_type,
                                  &(string_init_t){.str = name});
  return param;
}

void c_ir_param_dispose(allocator_t allocator, c_ir_param_t *param) {
  if (!param || !*param) return;
  c_ir_param_t p = *param;
  c_type_dispose(allocator, &p->type);
  allocator_free(allocator, &p->name);
  allocator_free(allocator, param);
}

c_ir_function_def_t c_ir_function_def_create(allocator_t allocator,
                                                c_type_t return_type,
                                                const char *name,
                                                vec_t params,
                                                bool is_static, bool is_inline,
                                                bool is_hidden, bool is_artificial,
                                                c_ir_node_t body,
                                                location_t source_loc) {
  c_ir_function_def_t node = allocator_alloc(allocator, sizeof(struct _c_ir_function_def_t));
  node->kind = C_IR_FUNCTION_DEF;
  node->source_loc = source_loc;
  node->return_type = return_type;
  node->name = allocator_create(allocator, &g_string_type,
                                 &(string_init_t){.str = name});
  node->params = params;
  node->is_static = is_static;
  node->is_inline = is_inline;
  node->is_hidden = is_hidden;
  node->is_artificial = is_artificial;
  node->body = body;
  return node;
}

void c_ir_function_def_dispose(allocator_t allocator, c_ir_function_def_t *node) {
  if (!node || !*node) return;
  c_ir_function_def_t n = *node;
  c_type_dispose(allocator, &n->return_type);
  allocator_free(allocator, &n->name);
  /* Dispose each param in the params vec */
  if (n->params) {
    size_t count = vec_get_size(n->params);
    for (size_t i = 0; i < count; i++) {
      c_ir_param_t param = vec_get(n->params, i);
      c_ir_param_dispose(allocator, &param);
    }
    allocator_free(allocator, &n->params);
  }
  if (n->body) c_ir_dispose(allocator, &n->body);
  allocator_free(allocator, node);
}

c_ir_function_decl_t c_ir_function_decl_create(allocator_t allocator,
                                                  c_type_t return_type,
                                                  const char *name,
                                                  vec_t params,
                                                  bool is_static, bool is_inline,
                                                  bool is_hidden, bool is_artificial,
                                                  location_t source_loc) {
  c_ir_function_decl_t node = allocator_alloc(allocator, sizeof(struct _c_ir_function_decl_t));
  node->kind = C_IR_FUNCTION_DECL;
  node->source_loc = source_loc;
  node->return_type = return_type;
  node->name = allocator_create(allocator, &g_string_type,
                                 &(string_init_t){.str = name});
  node->params = params;
  node->is_static = is_static;
  node->is_inline = is_inline;
  node->is_hidden = is_hidden;
  node->is_artificial = is_artificial;
  return node;
}

void c_ir_function_decl_dispose(allocator_t allocator, c_ir_function_decl_t *node) {
  if (!node || !*node) return;
  c_ir_function_decl_t n = *node;
  c_type_dispose(allocator, &n->return_type);
  allocator_free(allocator, &n->name);
  /* Dispose each param in the params vec */
  if (n->params) {
    size_t count = vec_get_size(n->params);
    for (size_t i = 0; i < count; i++) {
      c_ir_param_t param = vec_get(n->params, i);
      c_ir_param_dispose(allocator, &param);
    }
    allocator_free(allocator, &n->params);
  }
  allocator_free(allocator, node);
}
