#include "c/program.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/list.h"
#include "engine/context.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

cubec_value_t cubec_c_write_program(cubec_context_t self,
                                    cubec_ast_program_t program,
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
      char num[16];
      sprintf(num, "%" PRIuPTR " | ", node->loc.begin.line);
      char *line = cubec_location_get_line(node->loc, self->allocator);
      size_t len = strlen(line);
      size_t column = node->loc.begin.column - 1;
      len += strlen(num);
      char marks[len + 1];
      memset(marks, 0, len + 1);
      for (size_t id = 0; id < len; id++) {
        if (id < column + strlen(num)) {
          marks[id] = ' ';
        } else {
          marks[id] = '^';
        }
      }
      cubec_value_t err = cubec_context_create_error(
          self,
          "%s%s\n"
          "%s\n"
          "Top statement only support "
          "import,function,struct,enum,declaration,test",
          num, line, marks);
      cubec_allocator_free(self->allocator, line);
      return err;
    }
    it = cubec_list_node_next(it);
  }
  return self->value_undefined;
}
