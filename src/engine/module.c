#include "engine/module.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/string.h"
static void cubec_module_dispose(cubec_module_t self,
                                 cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->filename);
  cubec_allocator_free(allocator, self->dirname);
  cubec_allocator_free(allocator, self->variables);
  cubec_allocator_free(allocator, self->functions);
  cubec_allocator_free(allocator, self->structs);
  cubec_allocator_free(allocator, self->enums);
}
cubec_module_t cubec_create_module(cubec_allocator_t allocator,
                                   const char *filename, const char *dirname) {
  cubec_module_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_module_t),
                            (cubec_dispose_fn_t)cubec_module_dispose);
  self->filename = cubec_create_cstring(allocator, filename);
  self->dirname = cubec_create_cstring(allocator, dirname);
  cubec_array_initialize_t functions_initialize = {
      .autofree = true,
  };
  self->functions = cubec_create_array(allocator, &functions_initialize);
  cubec_array_initialize_t structs_initialize = {
      .autofree = true,
  };
  self->structs = cubec_create_array(allocator, &structs_initialize);
  cubec_array_initialize_t enums_initialize = {
      .autofree = true,
  };
  self->enums = cubec_create_array(allocator, &enums_initialize);
  cubec_array_initialize_t variables_initialize = {
      .autofree = true,
  };
  self->variables = cubec_create_array(allocator, &variables_initialize);
  return self;
}