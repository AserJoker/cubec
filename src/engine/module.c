#include "engine/module.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/compare.h"
#include "core/hash_map.h"
#include "core/path.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/function.h"
#include "engine/type.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
struct _module_t {
  ast_node_t node;
  value_t value;
  char *filename;
  char *dirname;
  char *source;
  hash_map_t indexed_functions;
  array_t functions;
  array_t structs;
  array_t errors;
};
static void module_dispose(module_t self, allocator_t allocator) {
  allocator_free(allocator, self->dirname);
  allocator_free(allocator, self->filename);
  allocator_free(allocator, self->source);
  allocator_free(allocator, self->node);
  allocator_free(allocator, self->functions);
  allocator_free(allocator, self->indexed_functions);
  allocator_free(allocator, self->structs);
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
  array_initialize_t structs_initialize = {
      .autofree = true,
  };
  self->structs = create_array(allocator, &structs_initialize);
  array_initialize_t errors_initialize = {
      .autofree = true,
  };
  self->errors = create_array(allocator, &errors_initialize);
  hash_map_initialize_t indexed_function_init = {
      .hash = (hash_fn_t)cstring_sdb,
      .compare = (compare_fn_t)strcmp,
      .autofree_key = false,
      .autofree_value = false,
  };
  self->indexed_functions = create_hash_map(allocator, &indexed_function_init);
  return self;
}
value_t module_get_value(module_t self) { return self->value; }
const char *module_get_filename(module_t self) { return self->filename; }
const char *module_get_dirname(module_t self) { return self->dirname; }
ast_node_t module_get_node(module_t self) { return self->node; }
void module_add_function(module_t self, struct _value_t *func) {
  array_push(self->functions, func);
  function_declar_t declar = *(function_declar_t *)value_get_data(func);
  hash_map_set(self->indexed_functions, (void *)declar->id, func, NULL, NULL);
}
void module_add_struct(module_t self, struct _value_t *stru) {
  array_push(self->structs, stru);
}
struct _value_t *module_get_function(module_t self, const char *id) {
  return hash_map_get(self->indexed_functions, id, NULL, NULL);
}
struct _value_t *module_get_struct(module_t self, const char *id) {
  for (size_t idx = 0; idx < array_get_size(self->structs); idx++) {
    value_t stru = array_get(self->functions, idx);
    type_t type = *(type_t *)value_get_data(stru);
    if (strcmp(type_get_id(type), id) == 0) {
      return stru;
    }
  }
  return NULL;
}
array_t module_get_functions(module_t self) { return self->functions; }
array_t module_get_structs(module_t self) { return self->structs; }
array_t module_get_errors(module_t self) { return self->errors; }
void module_add_error(module_t self, value_t err) {
  array_push(self->errors, err);
}
char *module_generator_func_id(module_t self, allocator_t allocator,
                               const char *base_id) {
  if (!module_get_function(self, base_id)) {
    return create_cstring(allocator, base_id);
  }
  for (size_t idx = 0; idx < UINT64_MAX; idx++) {
    size_t len = snprintf(NULL, 0, "%s_%" PRIxPTR, base_id, idx);
    char buf[len];
    sprintf(buf, "%s_%" PRIxPTR, base_id, idx);
    if (!module_get_function(self, buf)) {
      return create_cstring(allocator, buf);
    }
  }
  return NULL;
}