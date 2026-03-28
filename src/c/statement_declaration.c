#include "c/statement_declaration.h"
#include "ast/node.h"
#include "c/variable_declarator.h"
#include "core/array.h"
#include "core/string.h"
#include "engine/type.h"

cubec_value_t cubec_c_write_statement_declaration(cubec_context_t self,
                                                  cubec_ast_node_t sts,
                                                  const char *filename,
                                                  cubec_string_t *output) {
  cubec_ast_node_t list = cubec_map_get(sts->children, "declarations", NULL);
  for (size_t idx = 0; idx < cubec_array_get_size(list->items); idx++) {
    cubec_ast_node_t dec = cubec_array_get(list->items, idx);
    cubec_value_t err =
        cubec_c_write_variable_declarator(self, dec, filename, output);
    if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
      return err;
    }
    cubec_string_concat(*output, self->allocator, ";");
  }
  return self->value_undefined;
}