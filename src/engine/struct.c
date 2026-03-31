#include "engine/struct.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/string.h"
#include "engine/type.h"
#include "engine/value.h"
#include <string.h>

struct _cubec_struct_meta_t {
  char *name;
  size_t align;
  cubec_array_t fields;
  cubec_array_t attributes;
};
static void cubec_struct_meta_dispose(cubec_struct_meta_t self,
                                      cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->fields);
  cubec_allocator_free(allocator, self->attributes);
  cubec_allocator_free(allocator, self->name);
}
cubec_struct_meta_t cubec_create_struct_meta(cubec_allocator_t allocator,
                                             size_t align, const char *name) {
  cubec_struct_meta_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_struct_meta_t),
                            (cubec_dispose_fn_t)cubec_struct_meta_dispose);
  self->align = align;
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

struct _cubec_struct_field_t {
  char *name;
  cubec_type_t type;
  size_t offset;
};
typedef struct _cubec_struct_field_t *cubec_struct_field_t;
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

void cubec_struct_add_field(cubec_type_t self, cubec_allocator_t allocator,
                            const char *name, cubec_type_t type) {
  cubec_struct_meta_t meta = cubec_type_get_meta(self);
  size_t size = 0;
  size_t num_fields = cubec_array_get_size(meta->fields);
  if (num_fields) {
    size = cubec_type_get_size(self);
  }
  cubec_struct_field_t field =
      cubec_create_struct_field(allocator, name, type, size);
  cubec_array_push(meta->fields, field);
  size = field->offset + cubec_type_get_size(field->type);
  if (size % meta->align != 0) {
    size = size - size % meta->align + meta->align;
  }
  cubec_type_set_size(self, size);
}

struct _cubec_struct_attribute_t {
  char *name;
  cubec_value_t value;
};
typedef struct _cubec_struct_attribute_t *cubec_struct_attribute_t;
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
void cubec_struct_add_attribute(cubec_type_t self, cubec_allocator_t allocator,
                                const char *name, cubec_value_t value) {
  cubec_struct_attribute_t attr =
      cubec_create_struct_attribute(allocator, name, value);
  cubec_struct_meta_t meta = cubec_type_get_meta(self);
  cubec_array_push(meta->attributes, attr);
}
cubec_array_t cubec_struct_get_fields(cubec_type_t self,
                                      cubec_allocator_t allocator) {
  cubec_array_t fields = cubec_create_array(allocator, NULL);
  cubec_struct_meta_t meta = cubec_type_get_meta(self);
  size_t size = cubec_array_get_size(meta->fields);
  cubec_array_resize(fields, size);
  for (size_t idx = 0; idx < size; idx++) {
    cubec_struct_field_t field = cubec_array_get(meta->fields, idx);
    cubec_array_push(fields, field->name);
  }
  return fields;
}
cubec_array_t cubec_struct_get_attributes(cubec_type_t self,
                                          cubec_allocator_t allocator) {
  cubec_array_t attributes = cubec_create_array(allocator, NULL);
  cubec_struct_meta_t meta = cubec_type_get_meta(self);
  size_t size = cubec_array_get_size(meta->attributes);
  cubec_array_resize(attributes, size);
  for (size_t idx = 0; idx < size; idx++) {
    cubec_struct_attribute_t attr = cubec_array_get(meta->attributes, idx);
    cubec_array_push(attributes, attr->name);
  }
  return attributes;
}
cubec_type_t cubec_struct_get_field(cubec_type_t self, const char *name) {
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
size_t cubec_struct_get_offset(cubec_type_t self, const char *name) {
  cubec_struct_meta_t meta = cubec_type_get_meta(self);
  size_t size = cubec_array_get_size(meta->fields);
  for (size_t idx = 0; idx < size; idx++) {
    cubec_struct_field_t field = cubec_array_get(meta->fields, idx);
    if (strcmp(field->name, name) == 0) {
      return field->offset;
    }
  }
  return NULL;
}
cubec_value_t cubec_struct_get_attribute(cubec_type_t self, const char *name) {
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