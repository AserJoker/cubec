#include "astwriter/literal_identifier.h"
#include "core/allocator.h"
#include "core/any.h"

cubec_any_t cubec_write_ast_literal_identifier(
    cubec_allocator_t allocator,
    cubec_ast_literal_identifier_t literal_identifier) {
  cubec_any_t value =
      cubec_any_set_object(cubec_create_any(allocator), allocator);
  return value;
}