#include "engine/variable.h"
#include "core/allocator.h"
static void cubec_variable_dispose(cubec_variable_t variable,
                                   cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, variable->value);
}
cubec_variable_t cubec_create_variable(cubec_allocator_t allocator,
                                       cubec_value_t value, bool is_stack,
                                       bool is_const) {
  cubec_variable_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_variable_t),
                            (cubec_dispose_fn_t)cubec_variable_dispose);
  self->value = value;
  self->is_const = is_const;
  self->is_stack = is_stack;
  return self;
}