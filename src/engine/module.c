#include "engine/module.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/compare.h"
#include "core/map.h"
#include "core/path.h"
#include "core/string.h"
#include <string.h>
static void cubec_module_dispose(cubec_module_t self,
                                 cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->filename);
  cubec_allocator_free(allocator, self->dirname);
  cubec_allocator_free(allocator, self->variables);
  cubec_allocator_free(allocator, self->functions);
  cubec_allocator_free(allocator, self->dependences);
  cubec_allocator_free(allocator, self->types);
  cubec_allocator_free(allocator, self->data);
  cubec_allocator_free(allocator, self->node);
}
cubec_module_t cubec_create_module(cubec_allocator_t allocator,
                                   const char *filename) {
  cubec_module_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_module_t),
                            (cubec_dispose_fn_t)cubec_module_dispose);
  if (filename) {
    self->filename = cubec_create_cstring(allocator, filename);
    cubec_path_t name = cubec_create_path(allocator, filename);
    cubec_path_t parent = cubec_path_parent(name, allocator);
    self->dirname = cubec_path_to_string(parent, allocator);
    cubec_allocator_free(allocator, parent);
    cubec_allocator_free(allocator, name);
  } else {
    self->filename = NULL;
    cubec_path_t dirname = cubec_path_current(allocator);
    self->dirname = cubec_path_to_string(dirname, allocator);
    cubec_allocator_free(allocator, dirname);
  }
  cubec_array_initialize_t functions_initialize = {
      .autofree = true,
  };
  self->functions = cubec_create_array(allocator, &functions_initialize);
  cubec_array_initialize_t types_initialize = {
      .autofree = true,
  };
  self->types = cubec_create_array(allocator, &types_initialize);
  cubec_map_initialize_t variables_initialize = {
      .autofree_key = true,
      .autofree_value = false,
  };
  self->variables = cubec_create_map(allocator, &variables_initialize);
  cubec_map_initialize_t dep_initialize = {
      .compare = (cubec_compare_fn_t)strcmp,
  };
  self->dependences = cubec_create_map(allocator, &dep_initialize);
  self->data = cubec_create_string(allocator, NULL);
  self->node = NULL;
  return self;
}