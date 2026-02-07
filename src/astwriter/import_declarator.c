#include "astwriter/import_declarator.h"
#include "astwriter/node.h"
#include "core/value.h"

cubec_value_t
cubec_write_ast_import_declarator(cubec_allocator_t allocator,
                                  cubec_ast_import_declarator declarator) {
  cubec_value_t value =
      cubec_value_set_object(cubec_create_value(allocator), allocator);
  cubec_value_t alias = NULL;
  cubec_value_t identifier = NULL;
  if (declarator->alias) {
    alias = cubec_write_ast_node(declarator->alias, allocator);
  } else {
    alias = cubec_create_value(allocator);
  }
  if (declarator->identifier) {
    identifier = cubec_write_ast_node(declarator->identifier, allocator);
  } else {
    identifier = cubec_create_value(allocator);
  }
  cubec_value_set_field(value, allocator, "alias", alias);
  cubec_value_set_field(value, allocator, "identifier", identifier);
  return value;
}