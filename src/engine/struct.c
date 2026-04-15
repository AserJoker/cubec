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

struct _struct_meta_t {
  char *name;
  array_t fields;
  array_t attributes;
};
typedef struct _struct_meta_t *struct_meta_t;

static void struct_meta_dispose(struct_meta_t self, allocator_t allocator) {
  allocator_free(allocator, self->fields);
  allocator_free(allocator, self->attributes);
  allocator_free(allocator, self->name);
}
static struct_meta_t create_struct_meta(allocator_t allocator,
                                        const char *name) {
  struct_meta_t self = allocator_alloc(allocator, sizeof(struct _struct_meta_t),
                                       (dispose_fn_t)struct_meta_dispose);
  array_initialize_t fields_initialize = {
      .autofree = true,
  };
  self->fields = create_array(allocator, &fields_initialize);
  array_initialize_t attributes_initialize = {
      .autofree = true,
  };
  self->attributes = create_array(allocator, &attributes_initialize);
  if (name) {
    self->name = create_cstring(allocator, name);
  } else {
    self->name = NULL;
  }
  return self;
}
static bool struct_type_is_equal(type_t self, type_t another) {
  struct_meta_t self_meta = type_get_meta(self);
  struct_meta_t another_meta = type_get_meta(another);
  if (array_get_size(self_meta->fields) !=
      array_get_size(another_meta->fields)) {
    return false;
  }
  for (size_t idx = 0; idx < array_get_size(self_meta->fields); idx++) {
    struct_field_t self_field = array_get(self_meta->fields, idx);
    struct_field_t another_field = array_get(self_meta->fields, idx);
    if (self_field->offset != another_field->offset) {
      return false;
    }
    if (strcmp(self_field->name, another_field->name) != 0) {
      return false;
    }
    if (!type_is_equal(self_field->type, another_field->type)) {
      return false;
    }
  }
  return true;
}
static char *struct_type_to_string(type_t self, allocator_t allocator) {
  struct_meta_t meta = type_get_meta(self);
  size_t len = 32;
  if (meta->name) {
    return create_cstring(allocator, meta->name);
  }
  return create_cstring(allocator, "struct (unnamed){...}");
}
static value_t struct_to_string(value_t self, context_t ctx) {
  size_t len = 8;
  allocator_t allocator = context_get_allocator(ctx);
  type_t type = value_get_type(self);
  struct_meta_t meta = type_get_meta(type);
  char *type_name = struct_type_to_string(type, allocator);
  len += strlen(type_name);
  size_t num_fields = array_get_size(meta->fields);
  const char *fields[num_fields];
  for (size_t idx = 0; idx < num_fields; idx++) {
    struct_field_t field = array_get(meta->fields, idx);
    len += strlen(field->name) + 3;
    value_t value = value_get_field(self, ctx, field->name);
    value = value_to_string(value, ctx);
    fields[idx] = *(const char **)value_get_data(value);
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
    struct_field_t field = array_get(meta->fields, idx);
    strcpy(&str[offset], field->name);
    offset += strlen(field->name);
    str[offset++] = ' ';
    str[offset++] = '=';
    str[offset++] = ' ';
    strcpy(&str[offset], fields[idx]);
    offset += strlen(fields[idx]);
  }
  str[offset++] = '}';
  return create_str(ctx, str, NULL);
}
static value_t struct_get_length(value_t self, context_t ctx) {
  type_t type = value_get_type(self);
  struct_meta_t meta = type_get_meta(type);
  return create_u64(ctx, array_get_size(meta->fields), false, NULL);
}
static value_t struct_get_field(value_t self, context_t ctx, const char *name) {
  type_t type = value_get_type(self);
  struct_meta_t meta = type_get_meta(type);
  size_t num_fields = array_get_size(meta->fields);
  bool mutable = value_is_mutable(self);
  uint8_t *data = value_get_data(self);
  for (size_t idx = 0; idx < num_fields; idx++) {
    struct_field_t field = array_get(meta->fields, idx);
    if (strcmp(field->name, name) == 0) {
      if (!data) {
        return context_create_value(ctx, field->type, mutable, NULL, NULL);
      }
      return context_create_value(ctx, field->type, mutable,
                                  data + field->offset, NULL);
    }
  }
  return create_error(ctx, "no member named '%s' in value", name);
}
static value_t struct_set_field(value_t self, context_t ctx, const char *name,
                                value_t value) {
  type_t type = value_get_type(self);
  struct_meta_t meta = type_get_meta(type);
  size_t num_fields = array_get_size(meta->fields);
  uint8_t *data = value_get_data(self);
  bool mutable = value_is_mutable(self);
  type_t item_type = value_get_type(value);
  allocator_t allocator = context_get_allocator(ctx);
  if (!mutable) {
    return create_error(ctx, "Cannot assign to const variable");
  }
  for (size_t idx = 0; idx < num_fields; idx++) {
    struct_field_t field = array_get(meta->fields, idx);
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
      memcpy(data + field->offset, value_get_data(value),
             type_get_size(field->type));
      return context_get_undefined(ctx);
    }
  }
  return create_error(ctx, "No member named '%s' in value", name);
}
value_t create_struct_type(context_t ctx, size_t align, const char *name) {
  struct_meta_t meta = create_struct_meta(context_get_allocator(ctx), name);
  struct _type_operator_t opt = {
      .is_type_equal = struct_type_is_equal,
      .type_to_string = struct_type_to_string,
      .to_string = struct_to_string,
      .get_length = struct_get_length,
      .get_field = struct_get_field,
      .set_field = struct_set_field,
  };
  return context_create_type(ctx, VALUE_TYPE_STRUCT, 1, align, meta, &opt,
                             name);
}

static void struct_field_dispose(struct_field_t self, allocator_t allocator) {
  allocator_free(allocator, self->name);
}
static struct_field_t create_struct_field(allocator_t allocator,
                                          const char *name, type_t type,
                                          size_t offset) {
  struct_field_t self =
      allocator_alloc(allocator, sizeof(struct _struct_field_t),
                      (dispose_fn_t)struct_field_dispose);
  self->name = create_cstring(allocator, name);
  self->type = type;
  self->offset = offset;
  return self;
}

void struct_type_add_field(type_t self, allocator_t allocator, const char *name,
                           type_t type) {
  struct_meta_t meta = type_get_meta(self);
  size_t size = 0;
  size_t num_fields = array_get_size(meta->fields);
  size_t align = type_get_align(self);
  size_t field_align = type_get_align(type);
  if (align < field_align) {
    align = field_align;
  }
  if (num_fields) {
    struct_field_t field = array_get(meta->fields, num_fields - 1);
    size = field->offset + type_get_size(field->type);
  }
  if (size % field_align != 0) {
    size = size - size % field_align + field_align;
  }
  struct_field_t field = create_struct_field(allocator, name, type, size);
  array_push(meta->fields, field);
  size = field->offset + type_get_size(field->type);
  if (size % align != 0) {
    size = size - size % align + align;
  }
  type_set_align(self, align);
  type_set_size(self, size);
}

static void struct_attribute_dispose(struct_attribute_t self,
                                     allocator_t allocator) {
  allocator_free(allocator, self->name);
  allocator_free(allocator, self->value);
}
struct_attribute_t create_struct_attribute(allocator_t allocator,
                                           const char *name, value_t value) {
  struct_attribute_t self =
      allocator_alloc(allocator, sizeof(struct _struct_attribute_t),
                      (dispose_fn_t)struct_attribute_dispose);
  self->name = create_cstring(allocator, name);
  self->value = value_clone(allocator, value);
  return self;
}
void struct_type_add_attribute(type_t self, allocator_t allocator,
                               const char *name, value_t value) {
  struct_attribute_t attr = create_struct_attribute(allocator, name, value);
  struct_meta_t meta = type_get_meta(self);
  array_push(meta->attributes, attr);
}

type_t struct_type_get_field(type_t self, const char *name) {
  struct_meta_t meta = type_get_meta(self);
  size_t size = array_get_size(meta->fields);
  for (size_t idx = 0; idx < size; idx++) {
    struct_field_t field = array_get(meta->fields, idx);
    if (strcmp(field->name, name) == 0) {
      return field->type;
    }
  }
  return NULL;
}
size_t struct_type_get_offset(type_t self, const char *name) {
  struct_meta_t meta = type_get_meta(self);
  size_t size = array_get_size(meta->fields);
  for (size_t idx = 0; idx < size; idx++) {
    struct_field_t field = array_get(meta->fields, idx);
    if (strcmp(field->name, name) == 0) {
      return field->offset;
    }
  }
  return (size_t)-1;
}
value_t struct_type_get_attribute(type_t self, const char *name) {
  struct_meta_t meta = type_get_meta(self);
  size_t size = array_get_size(meta->attributes);
  for (size_t idx = 0; idx < size; idx++) {
    struct_attribute_t attr = array_get(meta->attributes, idx);
    if (strcmp(attr->name, name) == 0) {
      return attr->value;
    }
  }
  return NULL;
}
array_t struct_type_get_fields(type_t self) {
  struct_meta_t meta = type_get_meta(self);
  return meta->fields;
}
array_t struct_type_get_attributes(type_t self) {
  struct_meta_t meta = type_get_meta(self);
  return meta->attributes;
}