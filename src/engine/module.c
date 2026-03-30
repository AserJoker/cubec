#include "engine/module.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/path.h"
#include "core/string.h"
#include "engine/scope.h"
struct _cubec_module_t {
  char *filename;
  char *dirname;
  cubec_ast_node_t node;
  cubec_scope_t scope;
};
static void cubec_module_dispose(cubec_module_t module,
                                 cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, module->node);
  cubec_allocator_free(allocator, module->dirname);
  cubec_allocator_free(allocator, module->filename);
  cubec_allocator_free(allocator, module->scope);
}
cubec_module_t cubec_create_module(cubec_allocator_t allocator,
                                   cubec_ast_node_t node, const char *filename,
                                   cubec_scope_t parent) {
  cubec_path_t path = cubec_create_path(allocator, filename);
  cubec_path_t parent_path = cubec_path_parent(path, allocator);
  char *dirname = cubec_path_to_string(parent_path, allocator);
  cubec_allocator_free(allocator, parent_path);
  cubec_allocator_free(allocator, path);
  cubec_module_t module =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_module_t),
                            (cubec_dispose_fn_t)cubec_module_dispose);
  module->node = node;
  module->dirname = dirname;
  module->filename = cubec_create_cstring(allocator, filename);
  module->scope = cubec_create_scope(allocator, parent);
  return module;
}
const char *cubec_module_get_filename(cubec_module_t self) {
  return self->filename;
}
const char *cubec_module_get_dirname(cubec_module_t self) {
  return self->dirname;
}
cubec_ast_node_t cubec_module_get_node(cubec_module_t self) {
  return self->node;
}
cubec_scope_t cubec_module_get_scope(cubec_module_t self) {
  return self->scope;
}