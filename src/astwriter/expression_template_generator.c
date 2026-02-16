#include "astwriter/expression_template_generator.h"
#include "ast/node.h"
#include "astwriter/node.h"
#include "core/list.h"
#include "core/value.h"

cubec_value_t cubec_write_ast_expression_template_generator(
    cubec_allocator_t allocator,
    cubec_ast_expression_template_generator_t self) {
  cubec_value_t value = cubec_create_value(allocator);
  cubec_value_set_object(value, allocator);
  cubec_value_set_field(value, allocator, "template",
                        cubec_write_ast_node(self->temp, allocator));
  cubec_value_t args = cubec_create_value(allocator);
  cubec_value_set_array(args, allocator);
  cubec_list_node_t it = cubec_list_get_first(self->args);
  while (it != cubec_list_get_end(self->args)) {
    cubec_ast_node_t item = cubec_list_node_get(it);
    cubec_value_append(args, allocator, cubec_write_ast_node(item, allocator));
    it = cubec_list_node_next(it);
  }
  cubec_value_set_field(value, allocator, "args", args);
  return value;
}