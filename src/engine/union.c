#include "engine/union.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/type.h"
#include "engine/value.h"
#include <string.h>
struct _union_meta_t {
  array_t fields;
  array_t attributes;
  char *name;
};
typedef struct _union_meta_t *union_meta_t;
static void union_meta_dispose(union_meta_t self, allocator_t allocator) {
  allocator_free(allocator, self->name);
  allocator_free(allocator, self->fields);
  allocator_free(allocator, self->attributes);
}
static union_meta_t create_union_meta(allocator_t allocator, const char *name) {
  union_meta_t self = allocator_alloc(allocator, sizeof(struct _union_meta_t),
                                      (dispose_fn_t)union_meta_dispose);
  self->name = create_cstring(allocator, name);
  array_initialize_t fields_initialize = {
      .autofree = true,
  };
  self->fields = create_array(allocator, &fields_initialize);
  array_initialize_t attributes_initialize = {
      .autofree = true,
  };
  self->attributes = create_array(allocator, &attributes_initialize);
  return self;
}
static bool union_type_is_equal(type_t self, type_t another) {
  union_meta_t self_meta = type_get_meta(self);
  union_meta_t another_meta = type_get_meta(another);
  if (array_get_size(self_meta->fields) !=
      array_get_size(another_meta->fields)) {
    return false;
  }
  for (size_t idx = 0; idx < array_get_size(self_meta->fields); idx++) {
    union_field_t self_field = array_get(self_meta->fields, idx);
    union_field_t another_field = array_get(self_meta->fields, idx);
    if (strcmp(self_field->name, another_field->name) != 0) {
      return false;
    }
    if (!type_is_equal(self_field->type, another_field->type)) {
      return false;
    }
  }
  return true;
}
static char *union_type_to_string(type_t self, allocator_t allocator) {
  union_meta_t meta = type_get_meta(self);
  size_t len = 32;
  if (meta->name) {
    return create_cstring(allocator, meta->name);
  }
  return create_cstring(allocator, "union (unnamed){...}");
}

static value_t union_get_field(value_t self, context_t ctx, const char *name) {
  type_t type = value_get_type(self);
  union_meta_t meta = type_get_meta(type);
  size_t num_fields = array_get_size(meta->fields);
  uint8_t *data = value_get_data(self);
  bool mutable = value_is_mutable(self);
  for (size_t idx = 0; idx < num_fields; idx++) {
    union_field_t field = array_get(meta->fields, idx);
    if (strcmp(field->name, name) == 0) {
      return context_create_value(ctx, field->type, mutable, data, NULL);
    }
  }
  return create_error(ctx, "No member named '%s' in value", name);
}
static value_t union_set_field(value_t self, context_t ctx, const char *name,
                               value_t value) {
  type_t type = value_get_type(self);
  union_meta_t meta = type_get_meta(type);
  size_t num_fields = array_get_size(meta->fields);
  uint8_t *data = value_get_data(self);
  bool mutable = value_is_mutable(self);
  type_t item_type = value_get_type(value);
  allocator_t allocator = context_get_allocator(ctx);
  if (!mutable) {
    return create_error(ctx, "Cannot assign to const variable");
  }
  for (size_t idx = 0; idx < num_fields; idx++) {
    union_field_t field = array_get(meta->fields, idx);
    if (strcmp(field->name, name) == 0) {
      if (!type_is_equal(field->type, item_type)) {
        char *dst_type = type_to_string(field->type, allocator);
        char *src_type = type_to_string(item_type, allocator);
        value_t error =
            create_error(ctx, "Cannot assign '%s' to '%s'", dst_type, src_type);
        allocator_free(allocator, dst_type);
        allocator_free(allocator, src_type);
        return error;
      }
      if (!data) {
        return context_get_undefined(ctx);
      }
      memcpy(data, value_get_data(value), type_get_size(field->type));
      return context_get_undefined(ctx);
    }
  }
  return create_error(ctx, "No member named '%s' in value", name);
}
value_t create_union_type(context_t ctx, size_t align, const char *name) {
  union_meta_t meta = create_union_meta(context_get_allocator(ctx), name);
  struct _type_operator_t opt = {
      .is_type_equal = union_type_is_equal,
      .type_to_string = union_type_to_string,
      .get_field = union_get_field,
      .set_field = union_set_field,
  };
  return context_create_type(ctx, VALUE_TYPE_UNION, 1, align, meta, &opt, name);
}
static void union_field_dispose(union_field_t self, allocator_t allocator) {
  allocator_free(allocator, self->name);
}
static union_field_t create_union_field(allocator_t allocator, const char *name,
                                        type_t type) {
  union_field_t self = allocator_alloc(allocator, sizeof(struct _union_field_t),
                                       (dispose_fn_t)union_field_dispose);
  self->name = create_cstring(allocator, name);
  self->type = type;
  return self;
}
void union_type_add_field(type_t self, allocator_t allocator, const char *name,
                          type_t type) {
  union_field_t field = create_union_field(allocator, name, type);
  size_t align = type_get_align(self);
  if (align < type_get_align(type)) {
    align = type_get_align(type);
  }
  size_t size = type_get_size(self);
  if (size < type_get_size(type)) {
    size = type_get_size(type);
  }
  if (size % align != 0) {
    size = size - size % align + align;
  }
  type_set_align(self, align);
  type_set_size(self, size);
}
static void union_attribute_dispose(union_attribute_t self,
                                    allocator_t allocator) {
  allocator_free(allocator, self->name);
  allocator_free(allocator, self->value);
}
static union_attribute_t
create_union_attribute(allocator_t allocator, const char *name, value_t value) {
  union_attribute_t self =
      allocator_alloc(allocator, sizeof(struct _union_attribute_t),
                      (dispose_fn_t)union_attribute_dispose);
  self->name = create_cstring(allocator, name);
  self->value = value_clone(allocator, value);
  return self;
}
void union_type_add_attribute(type_t self, allocator_t allocator,
                              const char *name, value_t value) {
  union_meta_t meta = type_get_meta(self);
  union_attribute_t attr = create_union_attribute(allocator, name, value);
  array_push(meta->attributes, attr);
}
array_t union_type_get_fields(type_t self) {
  union_meta_t meta = type_get_meta(self);
  return meta->fields;
}
array_t union_type_get_attributes(type_t self) {
  union_meta_t meta = type_get_meta(self);
  return meta->attributes;
}
type_t union_type_get_field(type_t self, const char *name) {
  union_meta_t meta = type_get_meta(self);
  for (size_t idx; idx < array_get_size(meta->fields); idx++) {
    union_field_t field = array_get(meta->fields, idx);
    if (strcmp(field->name, name) == 0) {
      return field->type;
    }
  }
  return NULL;
}
value_t union_type_get_attribute(type_t self, const char *name) {
  union_meta_t meta = type_get_meta(self);
  for (size_t idx; idx < array_get_size(meta->attributes); idx++) {
    union_attribute_t attr = array_get(meta->attributes, idx);
    if (strcmp(attr->name, name) == 0) {
      return attr->value;
    }
  }
  return NULL;
}