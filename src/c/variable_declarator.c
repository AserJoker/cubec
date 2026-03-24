#include "c/variable_declarator.h"
#include "ast/type.h"
#include "c/type.h"
#include "core/allocator.h"
#include "core/location.h"
#include "core/string.h"
#include "engine/type.h"
#include "engine/value.h"
cubec_value_t cubec_c_write_variable_declarator(
    cubec_context_t self, cubec_ast_variable_declarator_t dec,
    const char *filename, cubec_string_t *output) {
  if (dec->type) {
    cubec_value_t err =
        cubec_c_write_type(self, (cubec_ast_type_t)dec->type, filename, output);
    if (err->type->kind == CUBEC_TYPE_KIND_ERROR) {
      return err;
    }
    char *name = cubec_location_get(dec->identifier->loc, self->allocator);
    cubec_string_concat(*output, self->allocator, " ");
    cubec_string_concat(*output, self->allocator, name);
    cubec_allocator_free(self->allocator, name);
    if (dec->initialize) {
      // TODO:
    }
  } else {
    // TODO:
  }
  return self->value_undefined;
}
