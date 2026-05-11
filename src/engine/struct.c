#include "engine/struct.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/string.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/function.h"
#include "engine/module.h"
#include "engine/ptr.h"
#include "engine/type.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdalign.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef struct _struct_meta_t *struct_meta_t;
struct _struct_meta_t {
  array_t fields;
  array_t attributes;
  bool locked_align;
  bool packed;
};

static void struct_meta_dispose(struct_meta_t self, allocator_t allocator) {
  allocator_free(allocator, self->fields);
  allocator_free(allocator, self->attributes);
}

static struct_meta_t create_struct_meta(allocator_t allocator) {
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
  self->locked_align = false;
  self->packed = false;
  return self;
}

static value_t struct_get_field(value_t self, context_t ctx, const char *name) {
  type_t type = value_get_type(self);
  struct_field_t field = struct_type_get_field(type, name);
  if (!field) {
    return create_error(ctx, "no member '%s' in type '%s'", name,
                        type_get_name(type));
  }
  if (!field->pub) {
    type_t self = context_get_self(ctx);
    if (self != type) {
      return create_error(ctx, "identifier '%s' is not visible", name);
    }
  }
  bool mut = value_is_mut(self);
  if (value_is_comptime(self)) {
    const void *data = value_get_data(self);
    value_t val = context_create_weak_value(
        ctx, field->type, (uint8_t *)data + field->offset, mut, NULL);
    return val;
  } else {
    return context_create_value(ctx, field->type, NULL, mut, false, NULL);
  }
}
static value_t struct_set_field(value_t self, context_t ctx, const char *name,
                                value_t value) {
  type_t type = value_get_type(self);
  struct_field_t field = struct_type_get_field(type, name);
  if (!field) {
    return create_error(ctx, "no member '%s' in type '%s'", name,
                        type_get_name(type));
  }
  if (!field->pub) {
    type_t self = context_get_self(ctx);
    if (self != type) {
      return create_error(ctx, "identifier '%s' is not visible", name);
    }
  }
  bool mut = value_is_mut(self);
  if (!field->mut || !mut) {
    return create_error(ctx, "value is not mutable");
  }
  if (value_is_comptime(self)) {
    const void *data = value_get_data(self);
    if (value_is_comptime(value)) {
      memcpy((uint8_t *)data + field->offset, value_get_data(value),
             type_get_size(field->type));
      return value;
    } else {
      return create_error(ctx, "value is not comptime");
    }
  } else {
    return value;
  }
}
static value_t struct_safe_convert(value_t self, context_t ctx, type_t type) {
  type_t value_type = value_get_type(self);
  if (type_get_kind(type) != TYPE_KIND_STRUCT ||
      type_get_size(type) != type_get_size(value_type) ||
      type_get_align(type) != type_get_align(value_type)) {
    return create_error(ctx, "cannot convert '%s' to '%s'",
                        type_get_name(value_type), type_get_name(type));
  }
  array_t src_fields = struct_type_get_fields(value_type);
  array_t dst_fields = struct_type_get_fields(type);
  if (array_get_size(src_fields) != array_get_size(dst_fields)) {
    return create_error(ctx, "cannot convert '%s' to '%s'",
                        type_get_name(value_type), type_get_name(type));
  }
  for (size_t idx = 0; idx < array_get_size(dst_fields); idx++) {
    struct_field_t src_field = array_get(src_fields, idx);
    struct_field_t dst_field = array_get(dst_fields, idx);
    if (src_field->offset != dst_field->offset ||
        strcmp(src_field->name, dst_field->name) != 0 ||
        !type_is_equal(src_field->type, dst_field->type)) {
      return create_error(ctx, "cannot convert '%s' to '%s'",
                          type_get_name(value_type), type_get_name(type));
    }
  }
  bool mut = value_is_mut(self);
  if (value_is_comptime(self)) {
    void *data = value_get_data(self);
    return context_create_weak_value(ctx, type, (void *)data, mut, NULL);
  } else {
    return context_create_value(ctx, type, NULL, mut, false, NULL);
  }
}

static value_t struct_convert(value_t self, context_t ctx, type_t type) {
  type_t value_type = value_get_type(self);
  if (type_get_kind(type) != TYPE_KIND_STRUCT ||
      type_get_size(type) != type_get_size(value_type) ||
      type_get_align(type) != type_get_align(value_type)) {
    return create_error(ctx, "cannot convert '%s' to '%s'",
                        type_get_name(value_type), type_get_name(type));
  }
  array_t src_fields = struct_type_get_fields(value_type);
  array_t dst_fields = struct_type_get_fields(type);
  if (array_get_size(src_fields) != array_get_size(dst_fields)) {
    return create_error(ctx, "cannot convert '%s' to '%s'",
                        type_get_name(value_type), type_get_name(type));
  }
  for (size_t idx = 0; idx < array_get_size(dst_fields); idx++) {
    struct_field_t src_field = array_get(src_fields, idx);
    struct_field_t dst_field = array_get(dst_fields, idx);
    if (src_field->offset != dst_field->offset ||
        strcmp(src_field->name, dst_field->name) != 0 ||
        !type_is_equal(src_field->type, dst_field->type)) {
      return create_error(ctx, "cannot convert '%s' to '%s'",
                          type_get_name(value_type), type_get_name(type));
    }
  }
  bool mut = value_is_mut(self);
  if (value_is_comptime(self)) {
    void *data = value_get_data(self);
    return context_create_weak_value(ctx, type, (void *)data, mut, NULL);
  } else {
    return context_create_value(ctx, type, NULL, mut, false, NULL);
  }
}

static bool struct_type_is_equal(type_t self, type_t another) {
  if (type_get_kind(another) != type_get_kind(self)) {
    return false;
  }
  struct_meta_t self_meta = type_get_meta(self);
  struct_meta_t another_meta = type_get_meta(another);
  if (array_get_size(self_meta->fields) !=
      array_get_size(another_meta->fields)) {
    return false;
  }
  for (size_t idx = 0; idx < array_get_size(self_meta->fields); idx++) {
    struct_field_t self_field = array_get(self_meta->fields, idx);
    struct_field_t another_field = array_get(another_meta->fields, idx);
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
static value_t struct_get(value_t self, context_t ctx, value_t key) {
  type_t type = value_get_type(self);
  struct_attribute_t attr = struct_type_get_attribute(type, "__get__");
  if (attr) {
    value_t ptr = create_ptr_value(ctx, self);
    value_t args[] = {ptr, key};
    return value_call(attr->value, ctx, 2, args);
  }
  return create_error(ctx, "no member __get__ found in '%s'",
                      type_get_name(type));
}
static value_t struct_set(value_t self, context_t ctx, value_t key,
                          value_t value) {
  type_t type = value_get_type(self);
  struct_attribute_t attr = struct_type_get_attribute(type, "__set__");
  if (attr) {
    value_t ptr = create_ptr_value(ctx, self);
    value_t args[] = {ptr, key, value};
    return value_call(attr->value, ctx, 3, args);
  }
  return create_error(ctx, "no member __set__ found in '%s'",
                      type_get_name(type));
}
static value_t struct_call(value_t self, context_t ctx, size_t argc,
                           value_t argv[]) {
  type_t type = value_get_type(self);
  struct_attribute_t attr = struct_type_get_attribute(type, "__call__");
  if (attr) {
    value_t ptr = create_ptr_value(ctx, self);
    value_t args[argc + 1];
    for (size_t idx = 0; idx < argc; idx++) {
      args[idx + 1] = argv[idx];
    }
    args[0] = ptr;
    return value_call(attr->value, ctx, argc + 1, args);
  }
  return create_error(ctx, "no member __call__ found in '%s'",
                      type_get_name(type));
}

type_t create_struct_type(context_t ctx, const char *name, size_t align) {

  char *id = NULL;
  allocator_t allocator = context_get_allocator(ctx);
  module_t module = context_get_module(ctx);
  const char *parent_name = NULL;
  value_t current_function = context_get_function(ctx);
  if (current_function) {
    value_t function_name = function_get_id(ctx, current_function);
    parent_name = *(const char **)value_get_data(function_name);
  } else {
    type_t self = context_get_self(ctx);
    if (self) {
      parent_name = type_get_id(self);
    } else {
      parent_name = NULL;
    }
  }
  const char *struct_name = name;
  if (!struct_name) {
    struct_name = "nonamed";
  }
  size_t len = strlen(struct_name) + 2;
  if (parent_name) {
    len += strlen(parent_name);
  }
  char base_fullname[len + 1];
  if (parent_name) {
    sprintf(base_fullname, "%s_%s", parent_name, struct_name);
  } else {
    sprintf(base_fullname, "%s", struct_name);
  }
  if (context_load_type(ctx, base_fullname)) {
    for (size_t idx = 0;; idx++) {
      size_t len = snprintf(NULL, 0, "%s_%" PRIuPTR, base_fullname, idx);
      char fullname[len + 1];
      sprintf(fullname, "%s_%" PRIuPTR, base_fullname, idx);
      if (!context_load_type(ctx, fullname)) {
        id = create_cstring(allocator, fullname);
        break;
      }
    }
  } else {
    id = create_cstring(allocator, base_fullname);
  }
  type_t self = context_load_type(ctx, id);
  if (!self) {
    allocator_t allocator = context_get_allocator(ctx);
    struct_meta_t meta = create_struct_meta(allocator);
    type_operator_t opt = {
        .addr_of = value_default_address_of,
        .get_field = struct_get_field,
        .set_field = struct_set_field,
        .convert = struct_convert,
        .safe_convert = struct_safe_convert,
        .type_eq = struct_type_is_equal,
        .get = struct_get,
        .set = struct_set,
        .call = struct_call,
    };
    self = create_type(allocator, TYPE_KIND_STRUCT, sizeof(int8_t), align, name,
                       id, &opt, meta);
    context_store_type(ctx, self);
    if (module) {
      module_add_type(module, self);
    }
  }
  allocator_free(allocator, id);
  return self;
}
void struct_type_lock_align(type_t self) {
  struct_meta_t meta = type_get_meta(self);
  meta->locked_align = true;
}
void struct_type_packed(type_t self) {
  struct_meta_t meta = type_get_meta(self);
  meta->packed = true;
}
static void struct_field_dispose(struct_field_t self, allocator_t allocator) {
  allocator_free(allocator, self->name);
}

static struct_field_t create_struct_field(allocator_t allocator,
                                          const char *name, size_t offset,
                                          type_t type, bool mut, bool pub) {
  struct_field_t self =
      allocator_alloc(allocator, sizeof(struct _struct_field_t),
                      (dispose_fn_t)struct_field_dispose);
  self->name = create_cstring(allocator, name);
  self->offset = offset;
  self->type = type;
  self->mut = mut;
  self->pub = pub;
  return self;
}

void struct_type_add_field(type_t self, allocator_t allocator, const char *name,
                           type_t type, bool mut, bool pub) {
  struct_meta_t meta = type_get_meta(self);
  size_t size = 0;
  size_t num_fields = array_get_size(meta->fields);
  size_t align = type_get_align(self);
  size_t field_align = type_get_align(type);
  if (!meta->locked_align) {
    if (align < field_align) {
      align = field_align;
    }
  }
  if (num_fields) {
    struct_field_t field = array_get(meta->fields, num_fields - 1);
    size = field->offset + type_get_size(field->type);
  }
  if (!meta->packed) {
    if (size % field_align != 0) {
      size = size - size % field_align + field_align;
    }
  }
  struct_field_t field =
      create_struct_field(allocator, name, size, type, mut, pub);
  array_push(meta->fields, field);
  size = field->offset + type_get_size(field->type);
  if (size % align != 0) {
    size = size - size % align + align;
  }
  type_set_align(self, align);
  type_set_size(self, size);
}
array_t struct_type_get_fields(type_t self) {
  struct_meta_t meta = type_get_meta(self);
  return meta->fields;
}

struct_field_t struct_type_get_field(type_t self, const char *name) {
  struct_meta_t meta = type_get_meta(self);
  array_t fields = meta->fields;
  for (size_t idx = 0; idx < array_get_size(fields); idx++) {
    struct_field_t attr = array_get(fields, idx);
    if (strcmp(attr->name, name) == 0) {
      return attr;
    }
  }
  return NULL;
}
void struct_type_remove_field(type_t self, const char *name) {
  struct_meta_t meta = type_get_meta(self);
  array_t fields = meta->fields;
  for (size_t idx = 0; idx < array_get_size(fields); idx++) {
    struct_field_t attr = array_get(fields, idx);
    if (strcmp(attr->name, name) == 0) {
      array_del(fields, idx);
      return;
    }
  }
}
static void struct_attribute_dispose(struct_attribute_t self,
                                     allocator_t allocator) {
  allocator_free(allocator, self->name);
  allocator_free(allocator, self->value);
}
static struct_attribute_t create_struct_attribute(allocator_t allocator,
                                                  const char *name,
                                                  value_t value, bool pub) {
  struct_attribute_t self =
      allocator_alloc(allocator, sizeof(struct _struct_attribute_t),
                      (dispose_fn_t)struct_attribute_dispose);
  self->name = create_cstring(allocator, name);
  self->value = value_clone(value, allocator);
  self->pub = pub;
  return self;
}
void struct_type_add_attribute(type_t self, allocator_t allocator,
                               const char *name, value_t value, bool pub) {
  struct_attribute_t attr =
      create_struct_attribute(allocator, name, value, pub);
  struct_meta_t meta = type_get_meta(self);
  array_push(meta->attributes, attr);
}
array_t struct_type_get_attributes(type_t self) {
  struct_meta_t meta = type_get_meta(self);
  return meta->attributes;
}
struct_attribute_t struct_type_get_attribute(type_t self, const char *name) {
  struct_meta_t meta = type_get_meta(self);
  array_t attributes = meta->attributes;
  for (size_t idx = 0; idx < array_get_size(attributes); idx++) {
    struct_attribute_t attr = array_get(attributes, idx);
    if (strcmp(attr->name, name) == 0) {
      return attr;
    }
  }
  return NULL;
}
void struct_type_remove_attribute(type_t self, const char *name) {
  struct_meta_t meta = type_get_meta(self);
  array_t attributes = meta->attributes;
  for (size_t idx = 0; idx < array_get_size(attributes); idx++) {
    struct_attribute_t attr = array_get(attributes, idx);
    if (strcmp(attr->name, name) == 0) {
      array_del(attributes, idx);
      return;
    }
  }
}