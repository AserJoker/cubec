#include "astwriter/statement_import.h"
#include "ast/node.h"
#include "astwriter/node.h"
#include "core/allocator.h"
#include "core/list.h"
#include "core/location.h"
#include "core/value.h"
cubec_value_t
cubec_write_ast_statement_import(cubec_allocator_t allocator,
                                 cubec_ast_statement_import_t statement) {
  cubec_value_t value =
      cubec_value_set_object(cubec_create_value(allocator), allocator);
  char *src = cubec_location_get(statement->source->loc, allocator);
  cubec_value_t source = cubec_write_ast_node(statement->source, allocator);
  cubec_allocator_free(allocator, src);
  cubec_value_set_field(value, allocator, "source", source);
  cubec_value_t declarators =
      cubec_value_set_array(cubec_create_value(allocator), allocator);
  cubec_list_node_t it = cubec_list_get_first(statement->declarators);
  while (it != cubec_list_get_end(statement->declarators)) {
    cubec_ast_node_t node = cubec_list_node_get(it);
    cubec_value_t item = cubec_write_ast_node(node, allocator);
    cubec_value_append(declarators, allocator, item);
    it = cubec_list_node_next(it);
  }
  cubec_value_set_field(value, allocator, "declarators", declarators);
  return value;
}