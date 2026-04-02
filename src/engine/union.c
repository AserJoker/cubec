#include "engine/union.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#include <string.h>
struct _cubec_union_meta_t {
  cubec_array_t fields;
  cubec_array_t attributes;
  char *name;
};
typedef struct _cubec_union_meta_t *cubec_union_meta_t;
static void cubec_union_meta_dispose(cubec_union_meta_t self,
                                     cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->name);
  cubec_allocator_free(allocator, self->fields);
  cubec_allocator_free(allocator, self->attributes);
}
static cubec_union_meta_t cubec_create_union_meta(cubec_allocator_t allocator,
                                                  const char *name) {
  cubec_union_meta_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_union_meta_t),
                            (cubec_dispose_fn_t)cubec_union_meta_dispose);
  self->name = cubec_create_cstring(allocator, name);
  cubec_array_initialize_t fields_initialize = {
      .autofree = true,
  };
  self->fields = cubec_create_array(allocator, &fields_initialize);
  cubec_array_initialize_t attributes_initialize = {
      .autofree = true,
  };
  self->attributes = cubec_create_array(allocator, &attributes_initialize);
  return self;
}
cubec_type_t cubec_context_union_type(cubec_context_t ctx, size_t align,
                                      const char *name) {
  cubec_union_meta_t meta =
      cubec_create_union_meta(cubec_context_get_allocator(ctx), name);
  return cubec_context_create_type(ctx, CUBEC_VALUE_TYPE_UNION, 1, align, meta);
}
struct _cubec_union_field_t {
  char *name;
  cubec_type_t type;
};
typedef struct _cubec_union_field_t *cubec_union_field_t;
static void cubec_union_field_dispose(cubec_union_field_t self,
                                      cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->name);
}
static cubec_union_field_t cubec_create_union_field(cubec_allocator_t allocator,
                                                    const char *name,
                                                    cubec_type_t type) {
  cubec_union_field_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_union_field_t),
                            (cubec_dispose_fn_t)cubec_union_field_dispose);
  self->name = cubec_create_cstring(allocator, name);
  self->type = type;
  return self;
}
void cubec_union_type_add_field(cubec_type_t self, cubec_allocator_t allocator,
                                const char *name, cubec_type_t type) {
  cubec_union_field_t field = cubec_create_union_field(allocator, name, type);
  size_t align = cubec_type_get_align(self);
  if (align < cubec_type_get_align(type)) {
    align = cubec_type_get_align(type);
  }
  size_t size = cubec_type_get_size(self);
  if (size < cubec_type_get_size(type)) {
    size = cubec_type_get_size(type);
  }
  if (size % align != 0) {
    size = size - size % align + align;
  }
  cubec_type_set_align(self, align);
  cubec_type_set_size(self, size);
}
struct _cubec_union_attribute_t {
  char *name;
  cubec_value_t value;
};
typedef struct _cubec_union_attribute_t *cubec_union_attribute_t;
static void cubec_union_attribute_dispose(cubec_union_attribute_t self,
                                          cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->name);
  cubec_allocator_free(allocator, self->value);
}
static cubec_union_attribute_t
cubec_create_union_attribute(cubec_allocator_t allocator, const char *name,
                             cubec_value_t value) {
  cubec_union_attribute_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_union_attribute_t),
                            (cubec_dispose_fn_t)cubec_union_attribute_dispose);
  self->name = cubec_create_cstring(allocator, name);
  self->value = cubec_value_clone(allocator, value);
  return self;
}
void cubec_union_type_add_attribute(cubec_type_t self,
                                    cubec_allocator_t allocator,
                                    const char *name, cubec_value_t value) {
  cubec_union_meta_t meta = cubec_type_get_meta(self);
  cubec_union_attribute_t attr =
      cubec_create_union_attribute(allocator, name, value);
  cubec_array_push(meta->attributes, attr);
}
cubec_array_t cubec_union_type_get_fields(cubec_type_t self,
                                          cubec_allocator_t allocator) {
  cubec_union_meta_t meta = cubec_type_get_meta(self);
  cubec_array_t fields = cubec_create_array(allocator, NULL);
  for (size_t idx; idx < cubec_array_get_size(meta->fields); idx++) {
    cubec_union_field_t field = cubec_array_get(meta->fields, idx);
    cubec_array_push(fields, field->name);
  }
  return fields;
}
cubec_array_t cubec_union_type_get_attributes(cubec_type_t self,
                                              cubec_allocator_t allocator) {
  cubec_union_meta_t meta = cubec_type_get_meta(self);
  cubec_array_t attributes = cubec_create_array(allocator, NULL);
  for (size_t idx; idx < cubec_array_get_size(meta->attributes); idx++) {
    cubec_union_attribute_t attr = cubec_array_get(meta->attributes, idx);
    cubec_array_push(attributes, attr->name);
  }
  return attributes;
}
cubec_type_t cubec_union_type_get_field(cubec_type_t self, const char *name) {
  cubec_union_meta_t meta = cubec_type_get_meta(self);
  for (size_t idx; idx < cubec_array_get_size(meta->fields); idx++) {
    cubec_union_field_t field = cubec_array_get(meta->fields, idx);
    if (strcmp(field->name, name) == 0) {
      return field->type;
    }
  }
  return NULL;
}
cubec_value_t cubec_union_type_get_attribute(cubec_type_t self,
                                             const char *name) {
  cubec_union_meta_t meta = cubec_type_get_meta(self);
  for (size_t idx; idx < cubec_array_get_size(meta->attributes); idx++) {
    cubec_union_attribute_t attr = cubec_array_get(meta->attributes, idx);
    if (strcmp(attr->name, name) == 0) {
      return attr->value;
    }
  }
  return NULL;
}