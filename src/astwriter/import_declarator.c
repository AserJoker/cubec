#include "astwriter/import_declarator.h"
#include "astwriter/node.h"
#include "core/any.h"

cubec_any_t
cubec_write_ast_import_declarator(cubec_allocator_t allocator,
                                  cubec_ast_import_declarator declarator) {
  cubec_any_t value =
      cubec_any_set_object(cubec_create_any(allocator), allocator);
  cubec_any_t alias = NULL;
  cubec_any_t identifier = NULL;
  if (declarator->alias) {
    alias = cubec_write_ast_node(declarator->alias, allocator);
  } else {
    alias = cubec_create_any(allocator);
  }
  if (declarator->identifier) {
    identifier = cubec_write_ast_node(declarator->identifier, allocator);
  } else {
    identifier = cubec_create_any(allocator);
  }
  cubec_any_set_field(value, allocator, "alias", alias);
  cubec_any_set_field(value, allocator, "identifier", identifier);
  return value;
}