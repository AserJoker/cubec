#include "engine/struct.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/numeric.h"
#include "engine/str.h"
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
static cubec_value_t cubec_struct_to_string(cubec_value_t self,
                                            cubec_context_t ctx) {
  size_t len = 8;
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  cubec_type_t type = cubec_value_get_type(self);
  cubec_struct_meta_t meta = cubec_type_get_meta(type);
  char *type_name = cubec_struct_type_to_string(type, allocator);
  len += strlen(type_name);
  size_t num_fields = cubec_array_get_size(meta->fields);
  const char *fields[num_fields];
  for (size_t idx = 0; idx < num_fields; idx++) {
    cubec_struct_field_t field = cubec_array_get(meta->fields, idx);
    len += strlen(field->name) + 3;
    cubec_value_t value = cubec_value_get_field(self, ctx, field->name);
    value = cubec_value_to_string(value, ctx);
    fields[idx] = *(const char **)cubec_value_get_data(value);
    len += strlen(fields[idx]) + 2;
  }
  char str[len];
  size_t offset = 0;
  strcpy(&str[offset], type_name);
  offset += strlen(type_name);
  str[offset++] = '{';
  for (size_t idx = 0; idx < num_fields; idx++) {
    if (idx != 0) {
      str[offset++] = ',';
      str[offset++] = ' ';
    }
    cubec_struct_field_t field = cubec_array_get(meta->fields, idx);
    strcpy(&str[offset], field->name);
    offset += strlen(field->name);
    str[offset++] = ' ';
    str[offset++] = '=';
    str[offset++] = ' ';
    strcpy(&str[offset], fields[idx]);
    offset += strlen(fields[idx]);
  }
  str[offset++] = '}';
  return cubec_create_str(ctx, str, NULL);
}
static cubec_value_t cubec_struct_get_length(cubec_value_t self,
                                             cubec_context_t ctx) {
  cubec_type_t type = cubec_value_get_type(self);
  cubec_struct_meta_t meta = cubec_type_get_meta(type);
  return cubec_create_uint64(ctx, cubec_array_get_size(meta->fields), false,
                             NULL);
}
static cubec_value_t cubec_struct_get_field(cubec_value_t self,
                                            cubec_context_t ctx,
                                            const char *name) {
  cubec_type_t type = cubec_value_get_type(self);
  cubec_struct_meta_t meta = cubec_type_get_meta(type);
  size_t num_fields = cubec_array_get_size(meta->fields);
  uint8_t *data = cubec_value_get_data(self);
  bool mutable = cubec_value_is_mutable(self);
  for (size_t idx = 0; idx < num_fields; idx++) {
    cubec_struct_field_t field = cubec_array_get(meta->fields, idx);
    if (strcmp(field->name, name) == 0) {
      return cubec_context_create_value(ctx, field->type, mutable,
                                        data + field->offset, NULL);
    }
  }
  return cubec_create_error(ctx, "No member named '%s' in value", name);
}
static cubec_value_t cubec_struct_set_field(cubec_value_t self,
                                            cubec_context_t ctx,
                                            const char *name,
                                            cubec_value_t value) {
  cubec_type_t type = cubec_value_get_type(self);
  cubec_struct_meta_t meta = cubec_type_get_meta(type);
  size_t num_fields = cubec_array_get_size(meta->fields);
  uint8_t *data = cubec_value_get_data(self);
  bool mutable = cubec_value_is_mutable(self);
  cubec_type_t item_type = cubec_value_get_type(value);
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  if (!mutable) {
    return cubec_create_error(ctx, "Cannot assign to const variable");
  }
  for (size_t idx = 0; idx < num_fields; idx++) {
    cubec_struct_field_t field = cubec_array_get(meta->fields, idx);
    if (strcmp(field->name, name) == 0) {
      if (!cubec_type_is_equal(field->type, item_type)) {
        char *dst_type = cubec_type_to_string(field->type, allocator);
        char *src_type = cubec_type_to_string(item_type, allocator);
        cubec_value_t error = cubec_create_error(
            ctx, "Cannot assign '%s' to '%s'", dst_type, src_type);
        cubec_allocator_free(allocator, dst_type);
        cubec_allocator_free(allocator, src_type);
        return error;
      }
      memcpy(data + field->offset, cubec_value_get_data(value),
             cubec_type_get_size(field->type));
      return cubec_context_get_undefined(ctx);
    }
  }
  return cubec_create_error(ctx, "No member named '%s' in value", name);
}
cubec_type_t cubec_context_create_struct_type(cubec_context_t ctx, size_t align,
                                              const char *name) {
  cubec_struct_meta_t meta =
      cubec_create_struct_meta(cubec_context_get_allocator(ctx), name);
  struct _cubec_type_operator_t opt = {
      .is_type_equal = cubec_struct_type_is_equal,
      .type_to_string = cubec_struct_type_to_string,
      .to_string = cubec_struct_to_string,
      .get_length = cubec_struct_get_length,
      .get_field = cubec_struct_get_field,
      .set_field = cubec_struct_set_field,
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