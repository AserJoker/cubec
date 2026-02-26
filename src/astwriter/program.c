#include "astwriter/program.h"
#include "ast/node.h"
#include "astwriter/node.h"
#include "core/allocator.h"
#include "core/any.h"
#include "core/list.h"

cubec_any_t cubec_write_ast_program(cubec_allocator_t allocator,
                                    cubec_ast_program_t program) {
  cubec_any_t value =
      cubec_any_set_object(cubec_create_any(allocator), allocator);
  cubec_any_t statements =
      cubec_any_set_array(cubec_create_any(allocator), allocator);
  cubec_list_node_t it = cubec_list_get_first(program->statements);
  while (it != cubec_list_get_end(program->statements)) {
    cubec_ast_node_t node = cubec_list_node_get(it);
    cubec_any_t item = cubec_write_ast_node(node, allocator);
    cubec_any_append(statements, allocator, item);
    it = cubec_list_node_next(it);
  }
  cubec_any_set_field(value, allocator, "statements", statements);
  return value;
}