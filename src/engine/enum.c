#include "engine/enum.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/string.h"
#include <stdbool.h>
static void cubec_enum_option_dispose(cubec_enum_option_t self,
                                      cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->name);
  cubec_allocator_free(allocator, self->value);
}

cubec_enum_option_t cubec_create_enum_option(cubec_allocator_t allocator,
                                             const char *name,
                                             cubec_value_t value) {
  cubec_enum_option_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_enum_option_t),
                            (cubec_dispose_fn_t)cubec_enum_option_dispose);
  self->name = cubec_create_cstring(allocator, name);
  self->value = value;
  return self;
}

static void cubec_enum_meta_dispose(cubec_enum_meta_t self,
                                    cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->options);
}

cubec_enum_meta_t cubec_create_enum_meta(cubec_allocator_t allocator,
                                         cubec_type_t type) {
  cubec_enum_meta_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_enum_meta_t),
                            (cubec_dispose_fn_t)cubec_enum_meta_dispose);
  self->type = type;
  cubec_array_initialize_t initiailze = {
      .autofree = true,
  };
  self->options = cubec_create_array(allocator, &initiailze);
  return self;
}