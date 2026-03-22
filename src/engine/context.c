#include "engine/context.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/compare.h"
#include "core/map.h"
#include "core/path.h"
#include "core/string.h"
#include "engine/array.h"
#include "engine/module.h"
#include "engine/ptr.h"
#include "engine/ref.h"
#include "engine/result.h"
#include "engine/scope.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const char *type_kind_strings[] = {
    "error",     // CUBEC_TYPE_KIND_ERROR,
    "void",      // CUBEC_TYPE_KIND_VOID
    "int8",      // CUBEC_TYPE_KIND_INT8
    "int16",     // CUBEC_TYPE_KIND_INT16
    "int32",     // CUBEC_TYPE_KIND_INT32
    "int64",     // CUBEC_TYPE_KIND_INT64
    "uint8",     // CUBEC_TYPE_KIND_UINT8
    "uint16",    // CUBEC_TYPE_KIND_UINT16
    "uint32",    // CUBEC_TYPE_KIND_UINT32
    "uint64",    // CUBEC_TYPE_KIND_UINT64
    "boolean",   // CUBEC_TYPE_KIND_BOOLEAN
    "str",       // CUBEC_TYPE_KIND_STR
    "opaque",    // CUBEC_TYPE_KIND_OPAQUE
    "ptr",       // CUBEC_TYPE_KIND_PTR
    "ptr_array", // CUBEC_TYPE_KIND_PTR_ARRAY
    "ref",       // CUBEC_TYPE_KIND_REF
    "array",     // CUBEC_TYPE_KIND_ARRAY
    "struct",    // CUBEC_TYPE_KIND_STRUCT
    "union",     // CUBEC_TYPE_KIND_UNION
    "enum",      // CUBEC_TYPE_KIND_ENUM
    "result",    // CUBEC_TYPE_KIND_RESULT
};

static void cubec_context_dispose(cubec_context_t self,
                                  cubec_allocator_t allocator) {
  while (self->current) {
    cubec_context_pop_scope(self);
  }
  cubec_allocator_free(allocator, self->modules);
  cubec_allocator_free(allocator, self->types);
}

cubec_context_t cubec_create_context(cubec_allocator_t allocator) {
  cubec_context_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_context_t),
                            (cubec_dispose_fn_t)cubec_context_dispose);
  self->allocator = allocator;
  cubec_map_initialize_t modules_initialize = {
      .autofree_key = false,
      .autofree_value = true,
      .compare = (cubec_compare_fn_t)strcmp,
  };
  self->modules = cubec_create_map(allocator, &modules_initialize);
  cubec_array_initialize_t strings_initialize = {
      .autofree = true,
      .compare = (cubec_compare_fn_t)strcmp,
  };
  self->strings = cubec_create_array(allocator, &strings_initialize);
  cubec_array_initialize_t types_initialize = {
      .autofree = true,
  };
  self->types = cubec_create_array(allocator, &types_initialize);
  self->root = cubec_create_scope(allocator, NULL);
  self->current = self->root;

  self->type_str = cubec_context_create_type(self, CUBEC_TYPE_KIND_ERROR,
                                             sizeof(const char *), NULL);
  self->type_void =
      cubec_context_create_type(self, CUBEC_TYPE_KIND_VOID, 0, NULL);
  self->value_undefined = cubec_context_create_value(self, self->type_void,
                                                     false, NULL, "undefined");
  self->type_int8 = cubec_context_create_type(self, CUBEC_TYPE_KIND_INT8,
                                              sizeof(int8_t), NULL);
  self->type_int16 = cubec_context_create_type(self, CUBEC_TYPE_KIND_INT16,
                                               sizeof(int16_t), NULL);
  self->type_int32 = cubec_context_create_type(self, CUBEC_TYPE_KIND_INT32,
                                               sizeof(int32_t), NULL);
  self->type_int64 = cubec_context_create_type(self, CUBEC_TYPE_KIND_INT64,
                                               sizeof(int64_t), NULL);
  self->type_uint8 = cubec_context_create_type(self, CUBEC_TYPE_KIND_UINT8,
                                               sizeof(uint8_t), NULL);
  self->type_uint16 = cubec_context_create_type(self, CUBEC_TYPE_KIND_UINT16,
                                                sizeof(uint16_t), NULL);
  self->type_uint32 = cubec_context_create_type(self, CUBEC_TYPE_KIND_UINT32,
                                                sizeof(uint32_t), NULL);
  self->type_uint64 = cubec_context_create_type(self, CUBEC_TYPE_KIND_UINT64,
                                                sizeof(uint64_t), NULL);
  self->type_boolean = cubec_context_create_type(self, CUBEC_TYPE_KIND_BOOLEAN,
                                                 sizeof(bool), NULL);
  self->type_str = cubec_context_create_type(self, CUBEC_TYPE_KIND_STR,
                                             sizeof(const char *), NULL);
  self->type_opaque = cubec_context_create_type(self, CUBEC_TYPE_KIND_OPAQUE,
                                                sizeof(void *), NULL);
  return self;
}
cubec_module_t cubec_context_get_module(cubec_context_t self,
                                        const char *name) {
  cubec_module_t mod = cubec_map_get(self->modules, name, NULL);
  if (!mod) {
    cubec_path_t path = cubec_create_path(self->allocator, name);
    cubec_path_t dirname = cubec_path_parent(path, self->allocator);
    char *dir = cubec_path_to_string(dirname, self->allocator);
    mod = cubec_create_module(self->allocator, name, dir);
    cubec_allocator_free(self->allocator, dir);
    cubec_allocator_free(self->allocator, dirname);
    cubec_allocator_free(self->allocator, path);
    cubec_map_set(self->modules, mod->filename, mod, NULL);
  }
  return mod;
}

cubec_type_t cubec_context_create_type(cubec_context_t self,
                                       cubec_type_kind_t kind, size_t size,
                                       void *meta) {
  cubec_type_t type = cubec_create_type(self->allocator, kind, size, meta);
  cubec_array_push(self->types, type);
  return type;
}
void cubec_context_push_scope(cubec_context_t self) {
  self->current = cubec_create_scope(self->allocator, self->current);
}
void cubec_context_pop_scope(cubec_context_t self) {
  for (size_t idx = 0; idx < cubec_array_get_size(self->current->defers);
       idx++) {
    // TODO: do defer
  }
  for (size_t idx = 0; idx < cubec_array_get_size(self->current->values);
       idx++) {
    // TODO: do __dispose__
  }
  cubec_scope_t scope = self->current;
  self->current = self->current->parent;
  cubec_allocator_free(self->allocator, scope);
}
cubec_type_t cubec_context_create_ptr_type(cubec_context_t self,
                                           cubec_type_t type, bool is_mutable,
                                           bool is_volatile) {
  cubec_ptr_meta_t meta =
      cubec_create_ptr_meta(self->allocator, type, is_mutable, is_volatile);
  return cubec_context_create_type(self, CUBEC_TYPE_KIND_PTR, sizeof(void *),
                                   meta);
}
cubec_type_t cubec_context_create_ref_type(cubec_context_t self,
                                           cubec_type_t type) {
  cubec_ref_meta_t meta = cubec_create_ref_meta(self->allocator, type);
  return cubec_context_create_type(self, CUBEC_TYPE_KIND_REF, sizeof(void *),
                                   meta);
}
cubec_type_t cubec_context_create_ptr_array_type(cubec_context_t self,
                                                 cubec_type_t type,
                                                 bool is_mutable,
                                                 bool is_volatile) {
  cubec_ptr_meta_t meta =
      cubec_create_ptr_meta(self->allocator, type, is_mutable, is_volatile);
  return cubec_context_create_type(self, CUBEC_TYPE_KIND_PTR_ARRAY,
                                   sizeof(void *), meta);
}
cubec_type_t cubec_context_create_array_type(cubec_context_t self,
                                             cubec_type_t type, size_t length) {
  cubec_array_meta_t meta =
      cubec_create_array_meta(self->allocator, type, length);
  return cubec_context_create_type(self, CUBEC_TYPE_KIND_ARRAY,
                                   type->size * length, meta);
}
cubec_type_t cubec_context_create_struct_type(cubec_context_t self,
                                              size_t align, const char *name) {
  cubec_struct_meta_t meta =
      cubec_create_struct_meta(self->allocator, align, name);
  return cubec_context_create_type(self, CUBEC_TYPE_KIND_STRUCT, 1, meta);
}
cubec_value_t cubec_context_add_struct_field(cubec_context_t self,
                                             cubec_type_t stru,
                                             const char *name,
                                             cubec_type_t type) {
  if (stru->kind != CUBEC_TYPE_KIND_STRUCT) {
    return cubec_context_create_error(self, "Cannot add field %s to %s", name,
                                      type_kind_strings[stru->kind]);
  }
  cubec_struct_meta_t meta = stru->meta;
  for (size_t idx = 0; idx < cubec_array_get_size(meta->fields); idx++) {
    cubec_struct_field_t field = cubec_array_get_index(meta->fields, idx);
    if (strcmp(field->name, name) == 0) {
      return cubec_context_create_error(self, "Duplicate field %s in struct",
                                        name);
    }
  }
  cubec_struct_add_field(stru, self->allocator, name, type);
  return self->value_undefined;
}
cubec_value_t cubec_context_add_struct_attribute(cubec_context_t self,
                                                 cubec_type_t stru,
                                                 const char *name,
                                                 cubec_value_t value) {
  if (stru->kind != CUBEC_TYPE_KIND_STRUCT) {
    return cubec_context_create_error(self, "Cannot add field %s to %s", name,
                                      type_kind_strings[stru->kind]);
  }
  cubec_struct_meta_t meta = stru->meta;
  for (size_t idx = 0; idx < cubec_array_get_size(meta->attributes); idx++) {
    cubec_struct_attribute_t attribute =
        cubec_array_get_index(meta->attributes, idx);
    if (strcmp(attribute->name, name) == 0) {
      return cubec_context_create_error(
          self, "Duplicate attribute %s in struct", name);
    }
  }
  cubec_struct_add_attribute(stru, self->allocator, name, value);
  return self->value_undefined;
}
cubec_type_t cubec_context_create_result_type(cubec_context_t self,
                                              cubec_type_t type,
                                              cubec_type_t etype) {
  cubec_result_meta_t meta =
      cubec_create_result_meta(self->allocator, type, etype);
  cubec_type_t result_type = cubec_context_create_type(
      self, CUBEC_TYPE_KIND_RESULT,
      (type->size > etype->size ? type->size : etype->size) + 1, meta);
  return result_type;
}
cubec_value_t cubec_context_create_int8(cubec_context_t self, int8_t value,
                                        bool is_mutable, const char *name) {
  return cubec_context_create_value(self, self->type_int8, is_mutable, &value,
                                    name);
}
cubec_value_t cubec_context_create_int16(cubec_context_t self, int16_t value,
                                         bool is_mutable, const char *name) {
  return cubec_context_create_value(self, self->type_int16, is_mutable, &value,
                                    name);
}
cubec_value_t cubec_context_create_int32(cubec_context_t self, int32_t value,
                                         bool is_mutable, const char *name) {
  return cubec_context_create_value(self, self->type_int32, is_mutable, &value,
                                    name);
}
cubec_value_t cubec_context_create_int64(cubec_context_t self, int64_t value,
                                         bool is_mutable, const char *name) {
  return cubec_context_create_value(self, self->type_int64, is_mutable, &value,
                                    name);
}
cubec_value_t cubec_context_create_uint8(cubec_context_t self, uint8_t value,
                                         bool is_mutable, const char *name) {
  return cubec_context_create_value(self, self->type_uint8, is_mutable, &value,
                                    name);
}
cubec_value_t cubec_context_create_uint16(cubec_context_t self, uint16_t value,
                                          bool is_mutable, const char *name) {
  return cubec_context_create_value(self, self->type_uint16, is_mutable, &value,
                                    name);
}
cubec_value_t cubec_context_create_uint32(cubec_context_t self, uint32_t value,
                                          bool is_mutable, const char *name) {
  return cubec_context_create_value(self, self->type_uint32, is_mutable, &value,
                                    name);
}
cubec_value_t cubec_context_create_uint64(cubec_context_t self, uint64_t value,
                                          bool is_mutable, const char *name) {
  return cubec_context_create_value(self, self->type_uint64, is_mutable, &value,
                                    name);
}
cubec_value_t cubec_context_create_boolean(cubec_context_t self, bool value,
                                           bool is_mutable, const char *name) {
  return cubec_context_create_value(self, self->type_boolean, is_mutable,
                                    &value, name);
}
cubec_value_t cubec_context_create_str(cubec_context_t self, const char *str,
                                       bool is_mutable, const char *name) {
  char *string = cubec_create_cstring(self->allocator, str);
  cubec_array_push(self->strings, string);
  return cubec_context_create_value(self, self->type_str, is_mutable, &string,
                                    name);
}
cubec_value_t cubec_context_create_opaque(cubec_context_t self, void *data,
                                          bool is_mutable, const char *name) {
  return cubec_context_create_value(self, self->type_opaque, is_mutable, &data,
                                    name);
}
cubec_value_t cubec_context_create_ptr(cubec_context_t self,
                                       cubec_value_t value, bool is_mutable,
                                       const char *name) {
  cubec_type_t type = value->type;
  void *data = value->data;
  if (type->kind == CUBEC_TYPE_KIND_REF) {
    cubec_ref_meta_t meta = type->meta;
    type = meta->type;
    data = *(void **)data;
  }
  cubec_type_t ptr_type =
      cubec_context_create_ptr_type(self, type, true, false);
  return cubec_context_create_value(self, ptr_type, is_mutable, &data, name);
}
cubec_value_t cubec_context_create_ptr_array(cubec_context_t self,
                                             cubec_value_t value,
                                             bool is_mutable,
                                             const char *name) {
  cubec_type_t type = value->type;
  void *data = value->data;
  if (type->kind == CUBEC_TYPE_KIND_REF) {
    cubec_ref_meta_t meta = type->meta;
    type = meta->type;
    data = *(void **)data;
  }
  cubec_type_t ptr_type =
      cubec_context_create_ptr_array_type(self, type, true, false);
  return cubec_context_create_value(self, ptr_type, is_mutable, &data, name);
}

cubec_value_t cubec_context_create_ref(cubec_context_t self,
                                       cubec_value_t value, bool is_mutable,
                                       const char *name) {
  cubec_type_t type = value->type;
  if (type->kind == CUBEC_TYPE_KIND_REF) {
    return value;
  }
  cubec_type_t ref_type = cubec_context_create_ref_type(self, type);
  return cubec_context_create_value(self, ref_type, is_mutable, &value->data,
                                    name);
}
cubec_value_t cubec_context_create_array(cubec_context_t self,
                                         cubec_type_t type, size_t length,
                                         bool is_mutable, const char *name) {
  cubec_type_t array_type = cubec_context_create_array_type(self, type, length);
  void *data =
      cubec_allocator_alloc(self->allocator, length * type->size, NULL);
  memset(data, 0, type->size * length);
  return cubec_context_create_value(self, array_type, is_mutable, data, name);
}
cubec_value_t cubec_context_create_struct(cubec_context_t self,
                                          cubec_type_t type, bool is_mutable,
                                          const char *name) {
  void *data = cubec_allocator_alloc(self->allocator, type->size, NULL);
  memset(data, 0, type->size);
  return cubec_context_create_value(self, type, is_mutable, data, name);
}
cubec_value_t cubec_context_set_index(cubec_context_t self, cubec_value_t arr,
                                      size_t idx, cubec_value_t value) {
  cubec_type_t dst_type = arr->type;
  void *dst = arr->data;
  cubec_type_t src_type = value->type;
  void *src = value->data;
  if (dst_type->kind == CUBEC_TYPE_KIND_REF) {
    cubec_ref_meta_t meta = dst_type->meta;
    dst_type = meta->type;
    dst = *(void **)dst;
  }
  if (src_type->kind == CUBEC_TYPE_KIND_REF) {
    cubec_ref_meta_t meta = src_type->meta;
    src_type = meta->type;
    src = *(void **)src;
  }
  // TODO: check type
  if (dst_type->kind == CUBEC_TYPE_KIND_ARRAY) {
    cubec_array_meta_t meta = dst_type->meta;
    if (idx >= meta->len) {
      return cubec_context_create_error(self, "Out of range");
    }
    uint8_t *offset = dst + meta->type->size * idx;
    memcpy(offset, src, src_type->size);
    return self->value_undefined;
  }
  if (dst_type->kind == CUBEC_TYPE_KIND_PTR_ARRAY) {
    cubec_ptr_meta_t meta = dst_type->meta;
    uint8_t *offset = dst + meta->type->size * idx;
    memcpy(offset, src, src_type->size);
    return self->value_undefined;
  }
  return cubec_context_create_error(
      self, "Cannot set %" PRIuPTR " from value(type = %s)", idx,
      type_kind_strings[arr->type->kind]);
}
cubec_value_t cubec_context_get_index(cubec_context_t self, cubec_value_t arr,
                                      size_t idx) {
  cubec_type_t type = arr->type;
  void *dst = arr->data;
  if (type->kind == CUBEC_TYPE_KIND_REF) {
    cubec_ref_meta_t meta = type->meta;
    type = meta->type;
    dst = *(void **)dst;
  }
  if (type->kind == CUBEC_TYPE_KIND_ARRAY) {
    cubec_array_meta_t meta = type->meta;
    if (meta->len <= idx) {
      return cubec_context_create_error(self, "Out of range");
    }
    uint8_t *start = dst + idx * meta->type->size;
    return cubec_context_create_value(self, meta->type, arr->is_mutable, start,
                                      NULL);
  }
  if (type->kind == CUBEC_TYPE_KIND_PTR_ARRAY) {
    cubec_ptr_meta_t meta = type->meta;
    uint8_t *start = *(uint8_t **)dst + idx * meta->type->size;
    return cubec_context_create_value(self, meta->type, arr->is_mutable, start,
                                      NULL);
  }
  return cubec_context_create_error(
      self, "Cannot get %" PRIuPTR " from value(type = %s)", idx,
      type_kind_strings[arr->type->kind]);
}
cubec_value_t cubec_context_set_field(cubec_context_t self, cubec_value_t obj,
                                      const char *name, cubec_value_t value) {
  cubec_type_t type = obj->type;
  void *dst = obj->data;
  if (type->kind == CUBEC_TYPE_KIND_REF) {
    cubec_ref_meta_t meta = type->meta;
    type = meta->type;
    dst = *(void **)dst;
  }
  if (type->kind == CUBEC_TYPE_KIND_PTR) {
    cubec_ptr_meta_t meta = type->meta;
    type = meta->type;
    dst = *(void **)dst;
  }
  if (type->kind == CUBEC_TYPE_KIND_STRUCT) {
    cubec_struct_meta_t meta = type->meta;
    for (size_t idx = 0; idx < cubec_array_get_size(meta->fields); idx++) {
      cubec_struct_field_t field = cubec_array_get_index(meta->fields, idx);
      if (strcmp(field->name, name) == 0) {
        memcpy(dst + field->offset, value->data, value->type->size);
        return self->value_undefined;
      }
    }
  }
  return cubec_context_create_error(self, "Cannot set %s from value(type = %s)",
                                    name, type_kind_strings[obj->type->kind]);
}

cubec_value_t cubec_context_get_field(cubec_context_t self, cubec_value_t obj,
                                      const char *name) {
  cubec_type_t type = obj->type;
  void *dst = obj->data;
  if (type->kind == CUBEC_TYPE_KIND_REF) {
    cubec_ref_meta_t meta = type->meta;
    type = meta->type;
    dst = *(void **)dst;
  }
  if (type->kind == CUBEC_TYPE_KIND_PTR) {
    cubec_ptr_meta_t meta = type->meta;
    type = meta->type;
    dst = *(void **)dst;
  }
  if (type->kind == CUBEC_TYPE_KIND_STRUCT) {
    cubec_struct_meta_t meta = type->meta;
    for (size_t idx = 0; idx < cubec_array_get_size(meta->fields); idx++) {
      cubec_struct_field_t field = cubec_array_get_index(meta->fields, idx);
      if (strcmp(field->name, name) == 0) {
        return cubec_context_create_value(self, field->type, obj->is_mutable,
                                          dst + field->offset, NULL);
      }
    }
  }
  return cubec_context_create_error(self, "Cannot get %s from value(type = %s)",
                                    name, type_kind_strings[obj->type->kind]);
}

cubec_value_t cubec_context_create_error(cubec_context_t self, const char *fmt,
                                         ...) {
  va_list args;
  va_start(args, fmt);
  size_t len = vsnprintf(NULL, 0, fmt, args);
  char *msg = cubec_allocator_alloc(self->allocator, len + sizeof(bool), NULL);
  vsprintf(msg, fmt, args);
  va_end(args);
  cubec_array_push(self->strings, msg);
  return cubec_context_create_value(self, self->type_error, false, &msg, NULL);
}

cubec_value_t cubec_context_create_value(cubec_context_t self,
                                         cubec_type_t type, bool is_mutable,
                                         const void *data, const char *name) {
  cubec_value_t value =
      cubec_create_value(self->allocator, type, is_mutable, data);
  cubec_array_push(self->current->values, value);
  if (name) {
    cubec_map_set(self->current->variables,
                  cubec_create_cstring(self->allocator, name), value, NULL);
  }
  return value;
}
cubec_value_t cubec_context_create_result(cubec_context_t self,
                                          cubec_type_t type,
                                          cubec_value_t value,
                                          cubec_value_t error, bool is_mutable,
                                          const char *name) {
  cubec_value_t val =
      cubec_context_create_value(self, type, is_mutable, NULL, name);
  cubec_result_data_t data = val->data;
  if (value) {
    data->flag = false;
    memcpy(data->data, value->data, value->type->size);
  } else {
    data->flag = true;
    memcpy(data->data, error->data, error->type->size);
  }
  return val;
}