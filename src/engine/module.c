#include "engine/module.h"
#include "core/allocator.h"
#include "core/compare.h"
#include "core/map.h"
#include "core/string.h"
#include <string.h>

static void cubec_module_dispose(cubec_module_t self,
                                 cubec_allocator_t allocaotr) {
  cubec_allocator_free(allocaotr, self->dirname);
  cubec_allocator_free(allocaotr, self->filename);
  cubec_allocator_free(allocaotr, self->source);
  cubec_allocator_free(allocaotr, self->node);
  cubec_allocator_free(allocaotr, self->exports);
}

cubec_module_t cubec_create_module(cubec_allocator_t allocator,
                                   const char *dirname, const char *filename,
                                   const char *source, cubec_ast_node_t node) {
  cubec_module_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_module_t),
                            (cubec_dispose_fn_t)cubec_module_dispose);
  self->dirname = cubec_create_cstring(allocator, dirname);
  self->filename = cubec_create_cstring(allocator, filename);
  cubec_map_initialize_t initialize = {
      .autofree_key = true,
      .autofree_value = false,
      .compare = (cubec_compare_fn_t)strcmp,
  };
  self->exports = cubec_create_map(allocator, &initialize);
  self->source = cubec_create_cstring(allocator, source);
  self->node = node;
  return self;
}