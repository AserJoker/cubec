#include "c/variable_declarator.h"
#include "ast/node.h"
#include "c/type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/map.h"
#include "core/string.h"
#include "engine/type.h"
#include "engine/value.h"
cubec_value_t cubec_c_write_variable_declarator(cubec_context_t self,
                                                cubec_ast_node_t dec,
                                                const char *filename,
                                                cubec_string_t *output) {
  cubec_ast_node_t type = cubec_map_get(dec->children, "type", NULL);
  if (type) {
    cubec_value_t err = cubec_c_write_type(self, type, filename, output);
    if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
      return err;
    }
    cubec_ast_node_t identifier =
        cubec_map_get(dec->children, "identifier", NULL);
    char *name = cubec_location_get(identifier->loc, self->allocator);
    cubec_string_concat(*output, self->allocator, " ");
    cubec_string_concat(*output, self->allocator, name);
    cubec_allocator_free(self->allocator, name);
    cubec_ast_node_t initialize =
        cubec_map_get(dec->children, "initialize", NULL);
    if (initialize) {
      // TODO:
    }
  } else {
    // TODO:
  }
  return self->value_undefined;
}
