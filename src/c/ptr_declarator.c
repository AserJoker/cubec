#include "c/ptr_declarator.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "c/type.h"
#include "core/array.h"
#include "core/map.h"
#include <string.h>

cubec_value_t cubec_c_write_ptr_declarator(cubec_context_t self,
                                           cubec_ast_node_t ptr,
                                           const char *filename,
                                           cubec_string_t *output) {
  cubec_ast_node_t type = cubec_map_get(ptr->children, "type", NULL);
  cubec_value_t err = cubec_c_write_type(self, type, filename, output);
  if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
    return err;
  }
  cubec_string_concat(*output, self->allocator, "*");
  cubec_ast_node_t list = cubec_map_get(ptr->children, "decorators", NULL);
  for (size_t idx = 0; idx < cubec_array_get_size(list->items); idx++) {
    if (idx != 0) {
      cubec_string_concat(*output, self->allocator, " ");
    }
    cubec_ast_node_t dec = cubec_array_get_index(list->items, idx);
    if (dec->type != CUBEC_NODE_TYPE_LITERAL_IDENTIFIER) {
      return cubec_context_create_compile_error(self, dec, filename,
                                                "Invalid pointer decorator");
    }
    char *d = cubec_location_get(dec->loc, self->allocator);
    if (strcmp(d, "const") != 0 && strcmp(d, "volatite") != 0) {
      cubec_allocator_free(self->allocator, d);
      return cubec_context_create_compile_error(self, dec, filename,
                                                "Unknown pointer decorator");
    }
    cubec_string_concat(*output, self->allocator, d);
    cubec_allocator_free(self->allocator, d);
  }
  return self->value_undefined;
}