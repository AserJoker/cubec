#include "engine/struct.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/string.h"
#include "engine/value.h"
static void cubec_struct_meta_dispose(cubec_struct_meta_t self,
                                      cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->name);
  cubec_allocator_free(allocator, self->fields);
  cubec_allocator_free(allocator, self->attributes);
}
cubec_struct_meta_t cubec_create_struct_meta(cubec_allocator_t allocator,
                                             size_t align, const char *name) {
  cubec_struct_meta_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_struct_meta_t),
                            (cubec_dispose_fn_t)cubec_struct_meta_dispose);
  self->align = align;
  if (name) {
    self->name = cubec_create_cstring(allocator, name);
  } else {
    self->name = NULL;
  }
  cubec_array_initialize_t initialize = {
      .autofree = true,
  };
  self->fields = cubec_create_array(allocator, &initialize);
  self->attributes = cubec_create_array(allocator, &initialize);
  return self;
}

static void cubec_struct_field_dispose(cubec_struct_field_t self,
                                       cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->name);
}

void cubec_struct_add_field(cubec_type_t stru, cubec_allocator_t allocator,
                            const char *name, cubec_type_t type) {
  cubec_struct_meta_t meta = stru->meta;
  cubec_struct_field_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_struct_field_t),
                            (cubec_dispose_fn_t)cubec_struct_field_dispose);
  self->name = cubec_create_cstring(allocator, name);
  self->type = type;
  self->offset = 0;
  size_t size = cubec_array_get_size(meta->fields);
  if (size == 0) {
    stru->size = 0;
  } else {
    cubec_struct_field_t last = cubec_array_get(meta->fields, size - 1);
    self->offset = last->offset + last->type->size;
    if (self->offset % self->type->size != 0) {
      self->offset =
          self->offset - (self->offset % self->type->size) + self->type->size;
    }
  }
  cubec_array_push(meta->fields, self);
  stru->size = self->offset + self->type->size;
  if (stru->size % meta->align != 0) {
    stru->size = stru->size - (stru->size % meta->align) + meta->align;
  }
}
static void cubec_struct_attribute_dispose(cubec_struct_attribute_t self,
                                           cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->name);
  cubec_allocator_free(allocator, self->value);
}
void cubec_struct_add_attribute(cubec_type_t stru, cubec_allocator_t allocator,
                                const char *name, cubec_value_t value) {
  cubec_struct_meta_t meta = stru->meta;
  cubec_struct_attribute_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_struct_attribute_t),
                            (cubec_dispose_fn_t)cubec_struct_attribute_dispose);
  self->name = cubec_create_cstring(allocator, name);
  self->value = cubec_create_value(allocator, value->type, value->is_mutable,
                                   value->data);
  cubec_array_push(meta->attributes, self);
}