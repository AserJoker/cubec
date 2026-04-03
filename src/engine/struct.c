#include "engine/struct.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#include <string.h>

struct _cubec_struct_meta_t {
  char *name;
  cubec_array_t fields;
  cubec_array_t attributes;
};
typedef struct _cubec_struct_meta_t *cubec_struct_meta_t;

static void cubec_struct_meta_dispose(cubec_struct_meta_t self,
                                      cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->fields);
  cubec_allocator_free(allocator, self->attributes);
  cubec_allocator_free(allocator, self->name);
}
static cubec_struct_meta_t cubec_create_struct_meta(cubec_allocator_t allocator,
                                                    const char *name) {
  cubec_struct_meta_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_struct_meta_t),
                            (cubec_dispose_fn_t)cubec_struct_meta_dispose);
  cubec_array_initialize_t fields_initialize = {
      .autofree = true,
  };
  self->fields = cubec_create_array(allocator, &fields_initialize);
  cubec_array_initialize_t attributes_initialize = {
      .autofree = true,
  };
  self->attributes = cubec_create_array(allocator, &attributes_initialize);
  if (name) {
    self->name = cubec_create_cstring(allocator, name);
  } else {
    self->name = NULL;
  }
  return self;
}
static bool cubec_struct_type_is_equal(cubec_type_t self,
                                       cubec_type_t another) {
  cubec_struct_meta_t self_meta = cubec_type_get_meta(self);
  cubec_struct_meta_t another_meta = cubec_type_get_meta(another);
  if (cubec_array_get_size(self_meta->fields) !=
      cubec_array_get_size(another_meta->fields)) {
    return false;
  }
  for (size_t idx = 0; idx < cubec_array_get_size(self_meta->fields); idx++) {
    cubec_struct_field_t self_field = cubec_array_get(self_meta->fields, idx);
    cubec_struct_field_t another_field =
        cubec_array_get(self_meta->fields, idx);
    if (self_field->offset != another_field->offset) {
      return false;
    }
    if (strcmp(self_field->name, another_field->name) != 0) {
      return false;
    }
    if (!cubec_type_is_equal(self_field->type, another_field->type)) {
      return false;
    }
  }
  return true;
}
static char *cubec_struct_type_to_string(cubec_type_t self,
                                         cubec_allocator_t allocator) {
  cubec_struct_meta_t meta = cubec_type_get_meta(self);
  size_t len = 32;
  if (meta->name) {
    return cubec_create_cstring(allocator, meta->name);
  }
  return cubec_create_cstring(allocator, "struct (unnamed){...}");
}
cubec_type_t cubec_context_create_struct_type(cubec_context_t ctx, size_t align,
                                              const char *name) {
  cubec_struct_meta_t meta =
      cubec_create_struct_meta(cubec_context_get_allocator(ctx), name);
  struct _cubec_type_operator_t opt = {
      .is_type_equal = cubec_struct_type_is_equal,
      .type_to_string = cubec_struct_type_to_string,
  };
  return cubec_context_create_type(ctx, CUBEC_VALUE_TYPE_STRUCT, 1, align, meta,
                                   &opt, name);
}

static void cubec_struct_field_dispose(cubec_struct_field_t self,
                                       cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->name);
}
static cubec_struct_field_t
cubec_create_struct_field(cubec_allocator_t allocator, const char *name,
                          cubec_type_t type, size_t offset) {
  cubec_struct_field_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_struct_field_t),
                            (cubec_dispose_fn_t)cubec_struct_field_dispose);
  self->name = cubec_create_cstring(allocator, name);
  self->type = type;
  self->offset = offset;
  return self;
}

void cubec_struct_type_add_field(cubec_type_t self, cubec_allocator_t allocator,
                                 const char *name, cubec_type_t type) {
  cubec_struct_meta_t meta = cubec_type_get_meta(self);
  size_t size = 0;
  size_t num_fields = cubec_array_get_size(meta->fields);
  size_t align = cubec_type_get_align(self);
  size_t field_align = cubec_type_get_align(type);
  if (align < field_align) {
    align = field_align;
  }
  if (num_fields) {
    cubec_struct_field_t field = cubec_array_get(meta->fields, num_fields - 1);
    size = field->offset + cubec_type_get_size(field->type);
  }
  if (size % field_align != 0) {
    size = size - size % field_align + field_align;
  }
  cubec_struct_field_t field =
      cubec_create_struct_field(allocator, name, type, size);
  cubec_array_push(meta->fields, field);
  size = field->offset + cubec_type_get_size(field->type);
  if (size % align != 0) {
    size = size - size % align + align;
  }
  cubec_type_set_align(self, align);
  cubec_type_set_size(self, size);
}

static void cubec_struct_attribute_dispose(cubec_struct_attribute_t self,
                                           cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->name);
  cubec_allocator_free(allocator, self->value);
}
cubec_struct_attribute_t
cubec_create_struct_attribute(cubec_allocator_t allocator, const char *name,
                              cubec_value_t value) {
  cubec_struct_attribute_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_struct_attribute_t),
                            (cubec_dispose_fn_t)cubec_struct_attribute_dispose);
  self->name = cubec_create_cstring(allocator, name);
  self->value = cubec_value_clone(allocator, value);
  return self;
}
void cubec_struct_type_add_attribute(cubec_type_t self,
                                     cubec_allocator_t allocator,
                                     const char *name, cubec_value_t value) {
  cubec_struct_attribute_t attr =
      cubec_create_struct_attribute(allocator, name, value);
  cubec_struct_meta_t meta = cubec_type_get_meta(self);
  cubec_array_push(meta->attributes, attr);
}

cubec_type_t cubec_struct_type_get_field(cubec_type_t self, const char *name) {
  cubec_struct_meta_t meta = cubec_type_get_meta(self);
  size_t size = cubec_array_get_size(meta->fields);
  for (size_t idx = 0; idx < size; idx++) {
    cubec_struct_field_t field = cubec_array_get(meta->fields, idx);
    if (strcmp(field->name, name) == 0) {
      return field->type;
    }
  }
  return NULL;
}
size_t cubec_struct_type_get_offset(cubec_type_t self, const char *name) {
  cubec_struct_meta_t meta = cubec_type_get_meta(self);
  size_t size = cubec_array_get_size(meta->fields);
  for (size_t idx = 0; idx < size; idx++) {
    cubec_struct_field_t field = cubec_array_get(meta->fields, idx);
    if (strcmp(field->name, name) == 0) {
      return field->offset;
    }
  }
  return (size_t)-1;
}
cubec_value_t cubec_struct_type_get_attribute(cubec_type_t self,
                                              const char *name) {
  cubec_struct_meta_t meta = cubec_type_get_meta(self);
  size_t size = cubec_array_get_size(meta->attributes);
  for (size_t idx = 0; idx < size; idx++) {
    cubec_struct_attribute_t attr = cubec_array_get(meta->attributes, idx);
    if (strcmp(attr->name, name) == 0) {
      return attr->value;
    }
  }
  return NULL;
}
cubec_array_t cubec_struct_type_get_fields(cubec_type_t self) {
  cubec_struct_meta_t meta = cubec_type_get_meta(self);
  return meta->fields;
}
cubec_array_t cubec_struct_type_get_attributes(cubec_type_t self) {
  cubec_struct_meta_t meta = cubec_type_get_meta(self);
  return meta->attributes;
}