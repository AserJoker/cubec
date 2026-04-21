#include "engine/union.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdalign.h>
#include <stdbool.h>
#include <string.h>

typedef struct _union_meta_t *union_meta_t;
struct _union_meta_t {
  char *name;
  array_t fields;
  array_t attributes;
};

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

static value_t union_get_field(value_t self, context_t ctx, const char *name) {
  type_t type = value_get_type(self);
  union_field_t field = union_type_get_field(type, name);
  if (!field) {
    return create_error(ctx, "no member '%s' in type '%s'", name,
                        type_get_name(type));
  }
  bool mut = value_is_mutable(self);
  if (!value_is_comptime(self)) {
    const void *data = value_get_data(self);
    value_t val =
        context_create_weak_value(ctx, field->type, (void *)data, mut, NULL);
    return val;
  } else {
    return context_create_value(ctx, field->type, NULL, mut, false, NULL);
  }
}
static value_t union_set_field(value_t self, context_t ctx, const char *name,
                               value_t value) {
  type_t type = value_get_type(self);
  union_field_t field = union_type_get_field(type, name);
  if (!field) {
    return create_error(ctx, "no member '%s' in type '%s'", name,
                        type_get_name(type));
  }
  bool mut = value_is_mutable(self);
  if (!value_is_comptime(self)) {
    const void *data = value_get_data(self);
    value_t val =
        context_create_weak_value(ctx, field->type, (void *)data, mut, NULL);
    return value_assigment(val, ctx, value);
  } else {
    return value;
  }
}

type_t create_union_type(context_t ctx, const char *name, const char *id,
                         size_t align) {
  type_t self = context_load_type(ctx, id);
  if (!self) {
    allocator_t allocator = context_get_allocator(ctx);
    if (!name) {
      name = "(unnamed)";
    }
    union_meta_t meta = create_union_meta(allocator, name);
    type_operator_t opt = {
        .get_field = union_get_field,
        .set_field = union_set_field,
    };
    self = create_type(allocator, TYPE_KIND_STRUCT, sizeof(int8_t), align, name,
                       id, &opt, meta);
    context_store_type(ctx, self);
  }
  return self;
}

static void union_field_dispose(union_field_t self, allocator_t allocator) {
  allocator_free(allocator, self->name);
}
static union_field_t create_union_field(allocator_t allocator, const char *name,
                                        size_t offset, type_t type) {
  union_field_t self = allocator_alloc(allocator, sizeof(struct _union_field_t),
                                       (dispose_fn_t)union_field_dispose);
  self->name = create_cstring(allocator, name);
  self->type = type;
  return self;
}

void union_type_add_field(type_t self, allocator_t allocator, const char *name,
                          type_t type) {
  union_meta_t meta = type_get_meta(self);
  size_t size = type_get_size(self);
  size_t num_fields = array_get_size(meta->fields);
  size_t align = type_get_align(self);
  size_t field_align = type_get_align(type);
  size_t field_size = type_get_size(type);
  if (align < field_align) {
    align = field_align;
  }
  if (size < field_size) {
    size = field_size;
  }
  if (size % align != 0) {
    size = size - size % align + align;
  }
  union_field_t field = create_union_field(allocator, name, size, type);
  array_push(meta->fields, field);
  type_set_align(self, align);
  type_set_size(self, size);
}
array_t union_type_get_fields(type_t self) {
  union_meta_t meta = type_get_meta(self);
  return meta->fields;
}

union_field_t union_type_get_field(type_t self, const char *name) {
  union_meta_t meta = type_get_meta(self);
  array_t fields = meta->fields;
  for (size_t idx = 0; idx < array_get_size(fields); idx++) {
    union_field_t attr = array_get(fields, idx);
    if (strcmp(attr->name, name) == 0) {
      return attr;
    }
  }
  return NULL;
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
  self->value = value_clone(value, allocator);
  return self;
}
void union_type_add_attribute(type_t self, allocator_t allocator,
                              const char *name, value_t value) {
  union_attribute_t attr = create_union_attribute(allocator, name, value);
  union_meta_t meta = type_get_meta(self);
  array_push(meta->attributes, attr);
}
array_t union_type_get_attributes(type_t self) {
  union_meta_t meta = type_get_meta(self);
  return meta->attributes;
}
union_attribute_t union_type_get_attribute(type_t self, const char *name) {
  union_meta_t meta = type_get_meta(self);
  array_t attributes = meta->attributes;
  for (size_t idx = 0; idx < array_get_size(attributes); idx++) {
    union_attribute_t attr = array_get(attributes, idx);
    if (strcmp(attr->name, name) == 0) {
      return attr;
    }
  }
  return NULL;
}