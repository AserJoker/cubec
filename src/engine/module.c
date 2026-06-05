#include "engine/module.h"
#include "ast/node.h"
#include "core/allocator.h"
#include "core/list.h"
#include "core/path.h"
#include "core/string.h"
static void module_dispsoe(module_t self, allocator_t allocator) {
  allocator_free(allocator, self->doc);
  allocator_free(allocator, self->dirname);
  allocator_free(allocator, self->filename);
  allocator_free(allocator, self->value);
  allocator_free(allocator, self->errors);
}

module_t create_module(allocator_t allocator, const char *filename,
                       value_t value, ast_doc_t doc) {
  module_t mod = allocator_alloc(allocator, sizeof(struct _module_t),
                                 (dispose_fn_t)module_dispsoe);
  mod->master = false;
  mod->value = value;
  mod->filename = create_cstring(allocator, filename);
  path_t fullpath = create_path(allocator, filename);
  path_t parent = path_parent(fullpath, allocator);
  allocator_free(allocator, fullpath);
  mod->dirname = path_to_string(parent, allocator);
  allocator_free(allocator, parent);
  mod->doc = doc;
  mod->errors = create_list(allocator, &(list_initialize_t){.autofree = true});
  return mod;
}