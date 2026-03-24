#include "c/ptr_declarator.h"
#include "ast/node_type.h"
#include "ast/type.h"
#include "c/type.h"
#include "c/writer.h"
#include <string.h>

cubec_value_t cubec_c_write_ptr_declarator(cubec_context_t self,
                                           cubec_ast_ptr_declarator_t ptr,
                                           const char *filename,
                                           cubec_string_t *output) {
  cubec_value_t err =
      cubec_c_write_type(self, (cubec_ast_type_t)ptr->type, filename, output);
  if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
    return err;
  }
  cubec_string_concat(*output, self->allocator, "*");
  cubec_ast_list_node_t list = (cubec_ast_list_node_t)ptr->decorators;
  for (cubec_list_node_t it = cubec_list_get_first(list->items);
       it != cubec_list_get_end(list->items); it = cubec_list_node_next(it)) {
    if (it != cubec_list_get_first(list->items)) {
      cubec_string_concat(*output, self->allocator, " ");
    }
    cubec_ast_node_t dec = cubec_list_node_get(it);
    if (dec->type != CUBEC_NODE_TYPE_LITERAL_IDENTIFIER) {
      return cubec_c_create_error(self, dec, filename,
                                  "Invalid pointer decorator");
    }
    char *d = cubec_location_get(dec->loc, self->allocator);
    if (strcmp(d, "const") != 0 && strcmp(d, "volatite") != 0) {
      cubec_allocator_free(self->allocator, d);
      return cubec_c_create_error(self, dec, filename,
                                  "Unknown pointer decorator");
    }
    cubec_string_concat(*output, self->allocator, d);
    cubec_allocator_free(self->allocator, d);
  }
  return self->value_undefined;
}