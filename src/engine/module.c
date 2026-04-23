#include "engine/module.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/path.h"
#include "core/string.h"
#include "engine/value.h"
struct _module_t {
  ast_node_t node;
  value_t value;
  char *filename;
  char *dirname;
  char *source;
  array_t functions;
  array_t errors;
};
static void module_dispose(module_t self, allocator_t allocator) {
  allocator_free(allocator, self->dirname);
  allocator_free(allocator, self->filename);
  allocator_free(allocator, self->source);
  allocator_free(allocator, self->node);
  allocator_free(allocator, self->functions);
  allocator_free(allocator, self->errors);
}
module_t create_module(allocator_t allocator, value_t value, ast_node_t node,
                       char *source, const char *filename) {
  module_t self = allocator_alloc(allocator, sizeof(struct _module_t),
                                  (dispose_fn_t)module_dispose);
  self->node = node;
  self->value = value;
  self->source = source;
  self->filename = create_cstring(allocator, filename);
  path_t fn = create_path(allocator, filename);
  path_t parent = path_parent(fn, allocator);
  allocator_free(allocator, fn);
  self->dirname = path_to_string(parent, allocator);
  allocator_free(allocator, parent);
  array_initialize_t functions_initialize = {
      .autofree = true,
  };
  self->functions = create_array(allocator, &functions_initialize);
  array_initialize_t errors_initialize = {
      .autofree = true,
  };
  self->errors = create_array(allocator, &errors_initialize);
  return self;
}
value_t module_get_value(module_t self) { return self->value; }
const char *module_get_filename(module_t self) { return self->filename; }
const char *module_get_dirname(module_t self) { return self->dirname; }
ast_node_t module_get_node(module_t self) { return self->node; }
void module_add_function(module_t self, struct _value_t *func) {
  array_push(self->functions, func);
}
array_t module_get_functions(module_t self) { return self->functions; }
array_t module_get_errors(module_t self) { return self->errors; }
void module_add_error(module_t self, value_t err) {
  array_push(self->errors, err);
}