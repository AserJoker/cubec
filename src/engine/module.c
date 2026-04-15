#include "engine/module.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/path.h"
#include "core/string.h"
#include "engine/value.h"
struct _module_t {
  char *filename;
  char *dirname;
  char *source;
  ast_node_t node;
  value_t value;
};
static void module_dispose(module_t module, allocator_t allocator) {
  allocator_free(allocator, module->node);
  allocator_free(allocator, module->dirname);
  allocator_free(allocator, module->filename);
  allocator_free(allocator, module->source);
}
module_t create_module(allocator_t allocator, ast_node_t node,
                       const char *filename, char *source, value_t value) {
  path_t path = create_path(allocator, filename);
  path_t parent_path = path_parent(path, allocator);
  char *dirname = path_to_string(parent_path, allocator);
  allocator_free(allocator, parent_path);
  allocator_free(allocator, path);
  module_t module = allocator_alloc(allocator, sizeof(struct _module_t),
                                    (dispose_fn_t)module_dispose);
  module->node = node;
  module->dirname = dirname;
  module->filename = create_cstring(allocator, filename);
  module->value = value;
  module->source = source;
  return module;
}
const char *module_get_filename(module_t self) { return self->filename; }
const char *module_get_dirname(module_t self) { return self->dirname; }
ast_node_t module_get_node(module_t self) { return self->node; }
value_t module_get_value(module_t self) { return self->value; }