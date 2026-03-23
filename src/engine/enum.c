#include "engine/enum.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/string.h"
#include "engine/value.h"
#include <string.h>

static void cubec_enum_meta_dispose(cubec_enum_meta_t self,
                                    cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->options);
  cubec_allocator_free(allocator, self->name);
}
cubec_enum_meta_t cubec_create_enum_meta(cubec_allocator_t allocator,
                                         cubec_type_t type, const char *name) {
  cubec_enum_meta_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_enum_meta_t),
                            (cubec_dispose_fn_t)cubec_enum_meta_dispose);
  self->type = type;
  cubec_array_initialize_t initialize = {
      .autofree = true,
  };
  self->options = cubec_create_array(allocator, &initialize);
  if (name) {
    self->name = cubec_create_cstring(allocator, name);
  } else {
    self->name = NULL;
  }
  return self;
}

static void cubec_enum_option_dispose(cubec_enum_option_t self,
                                      cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->name);
  cubec_allocator_free(allocator, self->value);
}

void cubec_add_enum_option(cubec_type_t self, cubec_allocator_t allocator,
                           const char *name, cubec_value_t value) {
  cubec_enum_meta_t meta = self->meta;
  cubec_enum_option_t option =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_enum_option_t),
                            (cubec_dispose_fn_t)cubec_enum_option_dispose);
  option->value = cubec_allocator_alloc(allocator, meta->type->size, NULL);
  memcpy(option->value, value->data, meta->type->size);
  option->name = cubec_create_cstring(allocator, name);
  cubec_array_push(meta->options, option);
}