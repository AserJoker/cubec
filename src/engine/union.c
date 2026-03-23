#include "engine/union.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/string.h"
static void cubec_union_meta_dispose(cubec_union_meta_t self,
                                     cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->name);
  cubec_allocator_free(allocator, self->fields);
  cubec_allocator_free(allocator, self->attributes);
}
cubec_union_meta_t cubec_create_union_meta(cubec_allocator_t allocator,
                                           size_t align, const char *name) {
  cubec_union_meta_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_union_meta_t),
                            (cubec_dispose_fn_t)cubec_union_meta_dispose);
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

static void cubec_union_field_dispose(cubec_union_field_t self,
                                      cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->name);
}

void cubec_union_add_field(cubec_type_t stru, cubec_allocator_t allocator,
                           const char *name, cubec_type_t type) {
  cubec_union_meta_t meta = stru->meta;
  cubec_union_field_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_union_field_t),
                            (cubec_dispose_fn_t)cubec_union_field_dispose);
  self->name = cubec_create_cstring(allocator, name);
  self->type = type;
  cubec_array_push(meta->fields, self);
  if (stru->size < type->size) {
    stru->size = type->size;
  }
}
static void cubec_union_attribute_dispose(cubec_union_attribute_t self,
                                          cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->name);
  cubec_allocator_free(allocator, self->value);
}
void cubec_union_add_attribute(cubec_type_t stru, cubec_allocator_t allocator,
                               const char *name, cubec_value_t value) {
  cubec_union_meta_t meta = stru->meta;
  cubec_union_attribute_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_union_attribute_t),
                            (cubec_dispose_fn_t)cubec_union_attribute_dispose);
  self->name = cubec_create_cstring(allocator, name);
  self->value = cubec_create_value(allocator, value->type, value->is_mutable,
                                   value->data);
  cubec_array_push(meta->attributes, self);
}