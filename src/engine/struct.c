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
#include "engine/value.h"
#include "engine/void.h"
#include <inttypes.h>
#include <stdalign.h>
#include <stdint.h>
#include <stdio.h>
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

static bool struct_type_equal(type_t self, type_t another) {
  if (another->kind == TYPE_KIND_STRUCT) {
    struct_meta_t self_meta = self->meta;
    struct_meta_t another_meta = another->meta;
    array_t self_fields = self_meta->fields;
    array_t another_fields = another_meta->fields;
    if (array_get_size(self_fields) != array_get_size(another_fields)) {
      return false;
    }
    for (size_t idx = 0; idx < array_get_size(self_fields); idx++) {
      struct_field_t self_field = array_get(self_fields, idx);
      struct_field_t another_field = array_get(another_fields, idx);
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
  return false;
}

static value_t struct_get_field(value_t self, context_t ctx,
                                const char *field) {
  type_t type = self->type;
  struct_field_t f = struct_type_get_field(type, field);
  if (!f) {
    return create_error(ctx, "no member '%s' in '%s'", field, type->name);
  }
  if (self->comptime) {
    return context_create_weak_value(
        ctx, f->type, (uint8_t *)self->data + f->offset, self->mut, NULL);
  }
  return context_create_value(ctx, f->type, self->mut, NULL);
}

static value_t struct_set_field(value_t self, context_t ctx, const char *field,
                                value_t value) {
  value_t item = struct_get_field(self, ctx, field);
  if (item->type->kind == TYPE_KIND_ERROR) {
    return item;
  }
  return value_assigment(item, ctx, value);
}
static value_t struct_get(value_t self, context_t ctx, value_t field) {
  value_t __get__ = struct_get_field(self, ctx, "__get__");
  value_t args[] = {self, field};
  return value_call(__get__, ctx, 2, args);
}
static value_t struct_set(value_t self, context_t ctx, value_t field,
                          value_t value) {
  value_t __set__ = struct_get_field(self, ctx, "__set__");
  value_t args[] = {self, field, value};
  return value_call(__set__, ctx, 3, args);
}
static value_t struct_len(value_t self, context_t ctx) {
  value_t __len__ = struct_get_field(self, ctx, "__len__");
  return value_call(__len__, ctx, 1, &self);
}
static value_t struct_call(value_t self, context_t ctx, size_t argc,
                           value_t argv[]) {
  value_t __call__ = struct_get_field(self, ctx, "__call__");
  value_t args[argc + 1];
  args[0] = self;
  for (size_t idx = 0; idx < argc; idx++) {
    args[idx + 1] = argv[idx];
  }
  return value_call(__call__, ctx, argc + 1, args);
}

type_t create_struct_type(context_t ctx, const char *name) {
  struct_meta_t meta = create_struct_meta(ctx->allocator);
  struct _type_operator_t opt = {
      .type_equal = struct_type_equal,
      .get_field = struct_get_field,
      .set_field = struct_set_field,
      .get = struct_get,
      .set = struct_set,
      .len = struct_len,
      .call = struct_call,
  };
  const char *id_name = name;
  if (!id_name) {
    id_name = "nonamed";
  }
  const char *parent_id = NULL;
  if (ctx->type == CONTEXT_TYPE_STRUCT) {
    parent_id = ctx->self->id;
  } else if (ctx->type == CONTEXT_TYPE_FUNCTION) {
    // TODO: function id;
  }
  size_t len = 0;
  if (parent_id) {
    len = snprintf(NULL, 0, "S%s%s", parent_id, id_name);
  } else {
    len = snprintf(NULL, 0, "S%s", id_name);
  }
  char base_id[len + 1];
  if (parent_id) {
    sprintf(base_id, "S%s%s", parent_id, id_name);
  } else {
    sprintf(base_id, "S%s", id_name);
  }
  module_t mod = ctx->mod;
  char *id = NULL;
  if (mod && hash_map_has(mod->structs, base_id, NULL, NULL)) {
    for (size_t idx = 0;; idx++) {
      size_t len = snprintf(NULL, 0, "%s%" PRIuPTR, base_id, idx);
      char id_data[len + 1];
      sprintf(id_data, "%s%" PRIuPTR, base_id, idx);
      if (!hash_map_has(mod->structs, id_data, NULL, NULL)) {
        id = create_cstring(ctx->allocator, id_data);
        break;
      }
    }
  } else {
    id = create_cstring(ctx->allocator, base_id);
  }
  type_t type = create_type(ctx->allocator, TYPE_KIND_STRUCT, name, id,
                            sizeof(char), alignof(struct {}), &opt, meta);
  allocator_free(ctx->allocator, id);
  context_store_type(ctx, type);
  if (mod) {
    hash_map_set(mod->structs, type->id, type, NULL, NULL);
    array_push(mod->indexed_structs, type->id);
  }
  return type;
}

static void struct_field_dispose(struct_field_t self, allocator_t allocator) {
  allocator_free(allocator, self->name);
}
static struct_field_t create_struct_field(allocator_t allocator,
                                          const char *name, type_t type,
                                          size_t offfset, bool pub, bool mut) {
  struct_field_t self =
      allocator_alloc(allocator, sizeof(struct _struct_field_t),
                      (dispose_fn_t)struct_field_dispose);
  self->name = create_cstring(allocator, name);
  self->offset = offfset;
  self->type = type;
  self->pub = pub;
  self->mut = mut;
  return self;
}
value_t struct_type_add_field(context_t ctx, type_t stru, const char *name,
                              type_t type, bool pub, bool mut) {
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
      create_struct_field(ctx->allocator, name, type, offset, pub, mut);
  size_t size = field->offset + field->type->align;
  if (size % stru->align != 0) {
    size = size - (size % stru->align) + stru->align;
  }
  array_push(meta->fields, field);
  stru->size = size;
  return create_comptime_void(ctx);
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
                               value_t value, bool pub) {
  struct_meta_t meta = stru->meta;
  for (size_t idx = 0; idx < array_get_size(meta->fields); idx++) {
    struct_field_t f = array_get(meta->fields, idx);
    if (strcmp(f->name, name) == 0) {
      return create_error(ctx, "duplicate member '%s'", name);
    }
  }
  value_t err = struct_type_add_attribute(ctx, stru, name, value, pub);
  if (err->type->kind == TYPE_KIND_ERROR) {
    return err;
  }
  hash_map_set(meta->methods, create_cstring(ctx->allocator, name), value, NULL,
               NULL);
  return create_comptime_void(ctx);
}
hash_map_t struct_type_get_methods(type_t stru) {
  struct_meta_t meta = stru->meta;
  return meta->methods;
}
struct_attribute_t struct_type_get_method(type_t stru, const char *name) {
  struct_meta_t meta = stru->meta;
  return hash_map_get(meta->methods, name, NULL, NULL);
}

static void struct_attribute_dispose(struct_attribute_t self,
                                     allocator_t allocator) {
  allocator_free(allocator, self->value);
}
static struct_attribute_t create_struct_attribute(allocator_t allocator,
                                                  value_t value, bool pub) {
  struct_attribute_t self =
      allocator_alloc(allocator, sizeof(struct _struct_attribute_t),
                      (dispose_fn_t)struct_attribute_dispose);
  self->value = value;
  self->pub = pub;
  return self;
}

value_t struct_type_add_attribute(context_t ctx, type_t stru, const char *name,
                                  value_t value, bool pub) {
  struct_meta_t meta = stru->meta;
  if (hash_map_get(meta->attributes, name, NULL, NULL)) {
    return create_error(ctx, "duplicate member '%s'", name);
  }
  struct_attribute_t attr = create_struct_attribute(ctx->allocator, value, pub);
  hash_map_set(meta->attributes, create_cstring(ctx->allocator, name), attr,
               NULL, NULL);
  return create_comptime_void(ctx);
}
hash_map_t struct_type_get_attributes(type_t stru) {
  struct_meta_t meta = stru->meta;
  return meta->attributes;
}
struct_attribute_t struct_type_get_attribute(type_t stru, const char *name) {
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
