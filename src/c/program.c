#include "c/program.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "c/writer.h"
#include "core/list.h"
#include "engine/context.h"
#include <inttypes.h>

cubec_value_t cubec_c_write_program(cubec_context_t self,
                                    cubec_ast_program_t program,
                                    const char *filename,
                                    cubec_string_t *output) {
  cubec_ast_list_node_t list = (cubec_ast_list_node_t)program->statements;
  cubec_list_t items = list->items;
  cubec_list_node_t it = cubec_list_get_first(items);
  while (it != cubec_list_get_end(items)) {
    cubec_ast_node_t node = cubec_list_node_get(it);
    if (node->type == CUBEC_NODE_TYPE_STATEMENT_IMPORT) {
    } else if (node->type == CUBEC_NODE_TYPE_STATEMENT_FUNCTION) {
    } else if (node->type == CUBEC_NODE_TYPE_STATEMENT_STRUCT) {
    } else if (node->type == CUBEC_NODE_TYPE_STATEMENT_ENUM) {
    } else if (node->type == CUBEC_NODE_TYPE_STATEMENT_DECLARATION) {
    } else if (node->type == CUBEC_NODE_TYPE_STATEMENT_TEST) {
    } else {
      return cubec_c_create_error(
          self, node, filename,
          "Top statement only support "
          "import,function,struct,enum,variable declaration,test");
    }
    it = cubec_list_node_next(it);
  }
  return self->value_undefined;
}
