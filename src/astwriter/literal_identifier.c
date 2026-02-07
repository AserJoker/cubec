#include "astwriter/literal_identifier.h"
#include "core/allocator.h"
#include "core/value.h"

cubec_value_t cubec_write_ast_literal_identifier(
    cubec_allocator_t allocator,
    cubec_ast_literal_identifier_t literal_identifier) {
  cubec_value_t value =
      cubec_value_set_object(cubec_create_value(allocator), allocator);
  return value;
}