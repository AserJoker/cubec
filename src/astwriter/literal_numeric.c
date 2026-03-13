#include "astwriter/literal_numeric.h"
#include "astwriter/node.h"
#include "core/any.h"

cubec_any_t cubec_write_ast_literal_numeric(cubec_allocator_t allocator,
                                            cubec_ast_literal_numeric_t self) {
  cubec_any_t value =
      cubec_any_set_object(cubec_create_any(allocator), allocator);
  if (self->flag) {
    cubec_any_set_field(value, allocator, "flag",
                        cubec_write_ast_node(self->flag, allocator));
  }
  cubec_any_set_field(value, allocator, "is_float",
                      cubec_any_set_boolean(cubec_create_any(allocator),
                                            allocator, self->is_float));
  cubec_any_set_field(value, allocator, "is_exp",
                      cubec_any_set_boolean(cubec_create_any(allocator),
                                            allocator, self->is_exp));
  return value;
}