#include "astwriter/program.h"
#include "ast/node.h"
#include "astwriter/node.h"
#include "core/allocator.h"
#include "core/list.h"
#include "core/value.h"

cubec_value_t cubec_write_ast_program(cubec_allocator_t allocator,
                                      cubec_ast_program_t program) {
  cubec_value_t value =
      cubec_value_set_object(cubec_create_value(allocator), allocator);
  cubec_value_t statements =
      cubec_value_set_array(cubec_create_value(allocator), allocator);
  cubec_list_node_t it = cubec_list_get_first(program->statements);
  while (it != cubec_list_get_end(program->statements)) {
    cubec_ast_node_t node = cubec_list_node_get(it);
    cubec_value_t item = cubec_write_ast_node(node, allocator);
    cubec_value_append(statements, allocator, item);
    it = cubec_list_node_next(it);
  }
  cubec_value_set_field(value, allocator, "statements", statements);
  return value;
}