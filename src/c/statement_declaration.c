#include "c/statement_declaration.h"
#include "ast/node.h"
#include "ast/variable_declarator.h"
#include "c/variable_declarator.h"
#include "core/list.h"
#include "core/string.h"
#include "engine/type.h"

cubec_value_t cubec_c_write_statement_declaration(
    cubec_context_t self, cubec_ast_statement_declaration_t sts,
    const char *filename, cubec_string_t *output) {
  cubec_ast_list_node_t list = (cubec_ast_list_node_t)sts->declarations;
  for (cubec_list_node_t it = cubec_list_get_first(list->items);
       it != cubec_list_get_end(list->items); it = cubec_list_node_next(it)) {
    cubec_ast_variable_declarator_t dec = cubec_list_node_get(it);
    cubec_value_t err =
        cubec_c_write_variable_declarator(self, dec, filename, output);
    if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
      return err;
    }
    cubec_string_concat(*output, self->allocator, ";");
  }
  return self->value_undefined;
}