#include "engine/union.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/error.h"
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
static bool cubec_union_type_is_equal(cubec_type_t self, cubec_type_t another) {
  cubec_union_meta_t self_meta = cubec_type_get_meta(self);
  cubec_union_meta_t another_meta = cubec_type_get_meta(another);
  if (cubec_array_get_size(self_meta->fields) !=
      cubec_array_get_size(another_meta->fields)) {
    return false;
  }
  for (size_t idx = 0; idx < cubec_array_get_size(self_meta->fields); idx++) {
    cubec_union_field_t self_field = cubec_array_get(self_meta->fields, idx);
    cubec_union_field_t another_field = cubec_array_get(self_meta->fields, idx);
    if (strcmp(self_field->name, another_field->name) != 0) {
      return false;
    }
    if (!cubec_type_is_equal(self_field->type, another_field->type)) {
      return false;
    }
  }
  return true;
}
static char *cubec_union_type_to_string(cubec_type_t self,
                                        cubec_allocator_t allocator) {
  cubec_union_meta_t meta = cubec_type_get_meta(self);
  size_t len = 32;
  if (meta->name) {
    return cubec_create_cstring(allocator, meta->name);
  }
  return cubec_create_cstring(allocator, "union (unnamed){...}");
}

static cubec_value_t cubec_union_get_field(cubec_value_t self,
                                           cubec_context_t ctx,
                                           const char *name) {
  cubec_type_t type = cubec_value_get_type(self);
  cubec_union_meta_t meta = cubec_type_get_meta(type);
  size_t num_fields = cubec_array_get_size(meta->fields);
  uint8_t *data = cubec_value_get_data(self);
  bool mutable = cubec_value_is_mutable(self);
  for (size_t idx = 0; idx < num_fields; idx++) {
    cubec_union_field_t field = cubec_array_get(meta->fields, idx);
    if (strcmp(field->name, name) == 0) {
      return cubec_context_create_value(ctx, field->type, mutable, data, NULL);
    }
  }
  return cubec_create_error(ctx, "No member named '%s' in value", name);
}
static cubec_value_t cubec_union_set_field(cubec_value_t self,
                                           cubec_context_t ctx,
                                           const char *name,
                                           cubec_value_t value) {
  cubec_type_t type = cubec_value_get_type(self);
  cubec_union_meta_t meta = cubec_type_get_meta(type);
  size_t num_fields = cubec_array_get_size(meta->fields);
  uint8_t *data = cubec_value_get_data(self);
  bool mutable = cubec_value_is_mutable(self);
  cubec_type_t item_type = cubec_value_get_type(value);
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  if (!mutable) {
    return cubec_create_error(ctx, "Cannot assign to const variable");
  }
  for (size_t idx = 0; idx < num_fields; idx++) {
    cubec_union_field_t field = cubec_array_get(meta->fields, idx);
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
      if (!data) {
        return cubec_context_get_undefined(ctx);
      }
      memcpy(data, cubec_value_get_data(value),
             cubec_type_get_size(field->type));
      return cubec_context_get_undefined(ctx);
    }
  }
  return cubec_create_error(ctx, "No member named '%s' in value", name);
}
cubec_value_t cubec_create_union_type(cubec_context_t ctx, size_t align,
                                      const char *name) {
  cubec_union_meta_t meta =
      cubec_create_union_meta(cubec_context_get_allocator(ctx), name);
  struct _cubec_type_operator_t opt = {
      .is_type_equal = cubec_union_type_is_equal,
      .type_to_string = cubec_union_type_to_string,
      .get_field = cubec_union_get_field,
      .set_field = cubec_union_set_field,
  };
  return cubec_context_create_type(ctx, CUBEC_VALUE_TYPE_UNION, 1, align, meta,
                                   &opt, name);
}
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
  return meta->fields;
}
cubec_array_t cubec_union_type_get_attributes(cubec_type_t self,
                                              cubec_allocator_t allocator) {
  cubec_union_meta_t meta = cubec_type_get_meta(self);
  return meta->attributes;
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