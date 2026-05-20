#include "engine/module.h"
#include "core/allocator.h"
#include "core/path.h"
#include "core/string.h"
static void module_dispsoe(module_t self, allocator_t allocator) {
  allocator_free(allocator, self->stru);
}
module_t create_module(allocator_t allocator, type_t stru,
                       const char *filename) {
  module_t mod = allocator_alloc(allocator, sizeof(struct _module_t),
                                 (dispose_fn_t)module_dispsoe);
  mod->master = false;
  mod->stru = stru;
  mod->filename = create_cstring(allocator, filename);
  path_t fullpath = create_path(allocator, filename);
  path_t parent = path_parent(fullpath, allocator);
  allocator_free(allocator, fullpath);
  mod->dirname = path_to_string(parent, allocator);
  allocator_free(allocator, parent);
  return mod;
}