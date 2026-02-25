#include "engine/interface.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/compare.h"
#include "core/map.h"
#include <string.h>
static void cubec_interface_type_dispose(cubec_interface_type_t self,
                                         cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->closure);
  cubec_allocator_free(allocator, self->args);
}
cubec_type_t cubec_create_interface_type(cubec_allocator_t allocator,
                                         const char *name,
                                         cubec_type_t return_type,
                                         bool variadic) {
  cubec_interface_type_t type =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_interface_type_t),
                            (cubec_dispose_fn_t)cubec_interface_type_dispose);
  type->super.kind = CUBEC_TYPE_KIND_INTERFACE;
  type->super.name = name;
  type->return_type = return_type;
  cubec_array_initialize_t args_initialize = {
      .autofree = false,
      .capacity = 0,
  };
  type->args = cubec_create_array(allocator, &args_initialize);
  type->variadic = variadic;
  cubec_map_initialize_t closure_initialize = {
      .autofree_key = false,
      .autofree_value = false,
      .compare = (cubec_compare_fn_t)strcmp,
  };
  type->closure = cubec_create_map(allocator, &closure_initialize);
  return &type->super;
}
static void cubec_interface_value_dispose(cubec_interface_value_t self,
                                          cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->closure);
}
cubec_value_t cubec_create_interface_value(cubec_allocator_t allocator,
                                           cubec_type_t type,
                                           cubec_ast_node_t *node) {
  cubec_interface_value_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_interface_value_t),
                            (cubec_dispose_fn_t)cubec_interface_value_dispose);
  self->super.type = type;
  cubec_map_initialize_t closure_initialize = {
      .autofree_key = false,
      .autofree_value = false,
      .compare = (cubec_compare_fn_t)strcmp,
  };
  self->closure = cubec_create_map(allocator, &closure_initialize);
  self->node = node;
  return &self->super;
}