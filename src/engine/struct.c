#include "engine/struct.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/compare.h"
#include "core/hash.h"
#include "core/hash_map.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/type.h"
#include <stdalign.h>
#include <string.h>
static void struct_meta_dispose(struct_meta_t self, allocator_t allocator) {
  allocator_free(allocator, self->attributes);
  allocator_free(allocator, self->fields);
  allocator_free(allocator, self->methods);
}
static struct_meta_t create_struct_meta(allocator_t allocator) {
  struct_meta_t self = allocator_alloc(allocator, sizeof(struct _struct_meta_t),
                                       (dispose_fn_t)struct_meta_dispose);
  self->fields = create_array(allocator, &(array_initialize_t){
                                             .autofree = true,
                                         });
  self->attributes =
      create_hash_map(allocator, &(hash_map_initialize_t){
                                     .autofree_key = true,
                                     .autofree_value = true,
                                     .hash = (hash_fn_t)(cstring_sdb),
                                     .compare = (compare_fn_t)(strcmp),
                                 });
  self->methods =
      create_hash_map(allocator, &(hash_map_initialize_t){
                                     .autofree_key = true,
                                     .autofree_value = false,
                                     .hash = (hash_fn_t)(cstring_sdb),
                                     .compare = (compare_fn_t)(strcmp),
                                 });
  self->packed = false;
  return self;
}
type_t create_struct_type(context_t ctx, const char *id, const char *name) {
  struct_meta_t meta = create_struct_meta(ctx->allocator);
  struct _type_operator_t opt = {};
  type_t type = create_type(ctx->allocator, TYPE_KIND_STRUCT, name, id,
                            sizeof(char), alignof(struct {}), &opt, meta);
  return type;
}

static void struct_field_dispose(struct_field_t self, allocator_t allocator) {
  allocator_free(allocator, self->name);
}
static struct_field_t create_struct_field(allocator_t allocator,
                                          const char *name, type_t type,
                                          size_t offfset) {
  struct_field_t self =
      allocator_alloc(allocator, sizeof(struct _struct_field_t),
                      (dispose_fn_t)struct_field_dispose);
  self->name = create_cstring(allocator, name);
  self->offset = offfset;
  self->type = type;
  return self;
}
value_t struct_type_add_field(context_t ctx, type_t stru, const char *name,
                              type_t type) {
  struct_meta_t meta = stru->meta;
  for (size_t idx = 0; idx < array_get_size(meta->fields); idx++) {
    struct_field_t f = array_get(meta->fields, idx);
    if (strcmp(f->name, name) == 0) {
      return create_error(ctx, "duplicate member '%s'", name);
    }
  }
  size_t offset = 0;
  if (array_get_size(meta->fields)) {
    struct_field_t last =
        array_get(meta->fields, array_get_size(meta->fields) - 1);
    offset = last->offset + last->type->size;
  }
  if (offset % type->align != 0) {
    offset = offset - (offset % type->align) + type->align;
  }
  struct_field_t field =
      create_struct_field(ctx->allocator, name, type, offset);
  size_t size = field->offset + field->type->align;
  if (size % stru->align != 0) {
    size = size - (size % stru->align) + stru->align;
  }
  array_push(meta->fields, field);
  stru->size = size;
  return context_get_undefined(ctx);
}
array_t struct_type_get_fields(type_t stru) {
  struct_meta_t meta = stru->meta;
  return meta->fields;
}
struct_field_t struct_type_get_field(type_t stru, const char *name) {
  struct_meta_t meta = stru->meta;
  for (size_t idx = 0; idx < array_get_size(meta->fields); idx++) {
    struct_field_t f = array_get(meta->fields, idx);
    if (strcmp(f->name, name) == 0) {
      return f;
    }
  }
  return NULL;
}
value_t struct_type_add_method(context_t ctx, type_t stru, const char *name,
                               value_t value) {
  struct_meta_t meta = stru->meta;
  for (size_t idx = 0; idx < array_get_size(meta->fields); idx++) {
    struct_field_t f = array_get(meta->fields, idx);
    if (strcmp(f->name, name) == 0) {
      return create_error(ctx, "duplicate member '%s'", name);
    }
  }
  value_t err = struct_type_add_attribute(ctx, stru, name, value);
  if (err->type->kind == TYPE_KIND_ERROR) {
    return err;
  }
  hash_map_set(meta->methods, create_cstring(ctx->allocator, name), value, NULL,
               NULL);
  return context_get_undefined(ctx);
}
hash_map_t struct_type_get_methods(type_t stru) {
  struct_meta_t meta = stru->meta;
  return meta->methods;
}
value_t struct_type_get_method(type_t stru, const char *name) {
  struct_meta_t meta = stru->meta;
  return hash_map_get(meta->methods, name, NULL, NULL);
}
value_t struct_type_add_attribute(context_t ctx, type_t stru, const char *name,
                                  value_t value) {
  struct_meta_t meta = stru->meta;
  if (hash_map_get(stru->meta, name, NULL, NULL)) {
    return create_error(ctx, "duplicate member '%s'", name);
  }
  hash_map_set(meta->attributes, create_cstring(ctx->allocator, name), value,
               NULL, NULL);
  return context_get_undefined(ctx);
}
hash_map_t struct_type_get_attributes(type_t stru) {
  struct_meta_t meta = stru->meta;
  return meta->attributes;
}
value_t struct_type_get_attribute(type_t stru, const char *name) {
  struct_meta_t meta = stru->meta;
  return hash_map_get(meta->attributes, name, NULL, NULL);
}
void struct_type_set_packed(type_t stru) {
  struct_meta_t meta = stru->meta;
  meta->packed = true;
  stru->align = 1;
  size_t offset = 0;
  stru->size = 0;
  for (size_t idx = 0; idx < array_get_size(meta->fields); idx++) {
    struct_field_t f = array_get(meta->fields, idx);
    f->offset = stru->size;
    stru->size += f->type->size;
  }
  if (stru->size == 0) {
    stru->size = 1;
  }
}
void struct_type_set_aligned(type_t stru, size_t aligned) {
  struct_meta_t meta = stru->meta;
  stru->align = aligned;
  if (array_get_size(meta->fields)) {
    struct_field_t f =
        array_get(meta->fields, array_get_size(meta->fields) - 1);
    stru->size = f->offset + f->type->size;
    if (stru->size % aligned != 0) {
      stru->size = stru->size - (stru->size % aligned) + aligned;
    }
  }
}
