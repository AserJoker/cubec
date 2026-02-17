#include "astwriter/function_self.h"
#include "astwriter/node.h"

cubec_value_t cubec_write_ast_function_self(cubec_allocator_t allocator,
                                            cubec_ast_function_self_t self) {

  cubec_value_t value =
      cubec_value_set_object(cubec_create_value(allocator), allocator);
  cubec_value_set_field(value, allocator, "argument",
                        cubec_write_ast_node(self->argument, allocator));
  return value;
}