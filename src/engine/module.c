#include "engine/module.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/path.h"
#include "core/string.h"
#include "engine/value.h"
struct _module_t {
  ast_node_t node;
  value_t value;
  char *filename;
  char *dirname;
};
static void module_dispose(module_t self, allocator_t allocator) {
  allocator_free(allocator, self->dirname);
  allocator_free(allocator, self->filename);
  allocator_free(allocator, self->node);
}
module_t create_module(allocator_t allocator, value_t value, ast_node_t node,
                       const char *filename) {
  module_t self = allocator_alloc(allocator, sizeof(struct _module_t),
                                  (dispose_fn_t)module_dispose);
  self->node = node;
  self->value = value;
  self->filename = create_cstring(allocator, filename);
  path_t fn = create_path(allocator, filename);
  path_t parent = path_parent(fn, allocator);
  allocator_free(allocator, fn);
  self->dirname = path_to_string(parent, allocator);
  allocator_free(allocator, parent);
  return self;
}
value_t module_get_value(module_t self) { return self->value; }
const char *module_get_filename(module_t self) { return self->filename; }
const char *module_get_dirname(module_t self) { return self->dirname; }