#include "c/literal_identifier.h"
#include "core/location.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/value.h"
cubec_value_t
cubec_c_write_literal_identifier(cubec_context_t self,
                                 cubec_ast_literal_identifier_t identifier,
                                 const char *filename, cubec_string_t *output) {
  char *name = cubec_location_get(identifier->super.loc, self->allocator);
  cubec_string_concat(*output, self->allocator, name);
  cubec_allocator_free(self->allocator, name);
  return self->value_undefined;
}