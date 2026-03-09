#include "engine/context.h"
#include "ast/program.h"
#include "astwriter/node.h"
#include "core/allocator.h"
#include "core/any.h"
#include "core/array.h"
#include "core/compare.h"
#include "core/list.h"
#include "core/map.h"
#include "core/string.h"
#include "engine/array.h"
#include "engine/enum.h"
#include "engine/function.h"
#include "engine/ptr.h"
#include "engine/scope.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/union.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void cubec_context_dispose(cubec_context_t self,
                                  cubec_allocator_t allocator) {
  while (self->current) {
    cubec_context_pop(self);
  }
  cubec_allocator_free(allocator, self->types);
  cubec_allocator_free(allocator, self->strings);
  cubec_allocator_free(allocator, self->functions);
}

static void cubec_context_init_types(cubec_context_t self) {
  cubec_list_initialize_t initialize = {
      .autofree = true,
  };
  self->types = cubec_create_list(self->allocator, &initialize);
  self->named_types.self_type =
      cubec_context_create_type(self, CUBEC_VALUE_TYPE_PTR, 0, "Self", NULL);
  self->named_types.undefined_type = cubec_context_create_type(
      self, CUBEC_VALUE_TYPE_UNDEFINED, 0, "undefined", NULL);
  self->named_types.int8_type = cubec_context_create_type(
      self, CUBEC_VALUE_TYPE_INT8, sizeof(int8_t), "int8", NULL);
  self->named_types.int16_type = cubec_context_create_type(
      self, CUBEC_VALUE_TYPE_INT16, sizeof(int16_t), "int16", NULL);
  self->named_types.int32_type = cubec_context_create_type(
      self, CUBEC_VALUE_TYPE_INT32, sizeof(int32_t), "int32", NULL);
  self->named_types.int64_type = cubec_context_create_type(
      self, CUBEC_VALUE_TYPE_INT64, sizeof(int64_t), "int64", NULL);
  self->named_types.uint8_type = cubec_context_create_type(
      self, CUBEC_VALUE_TYPE_UINT8, sizeof(uint8_t), "uint8", NULL);
  self->named_types.uint16_type = cubec_context_create_type(
      self, CUBEC_VALUE_TYPE_UINT16, sizeof(uint16_t), "uint16", NULL);
  self->named_types.uint32_type = cubec_context_create_type(
      self, CUBEC_VALUE_TYPE_UINT32, sizeof(uint32_t), "uint32", NULL);
  self->named_types.uint64_type = cubec_context_create_type(
      self, CUBEC_VALUE_TYPE_UINT64, sizeof(uint64_t), "uint64", NULL);
  self->named_types.float32_type = cubec_context_create_type(
      self, CUBEC_VALUE_TYPE_FLOAT32, sizeof(float), "float32", NULL);
  self->named_types.float64_type = cubec_context_create_type(
      self, CUBEC_VALUE_TYPE_FLOAT64, sizeof(double), "float64", NULL);
  self->named_types.boolean_type = cubec_context_create_type(
      self, CUBEC_VALUE_TYPE_BOOLEAN, sizeof(bool), "boolean", NULL);
  self->named_types.str_type = cubec_context_create_type(
      self, CUBEC_VALUE_TYPE_STR, sizeof(const char *), "str", NULL);
  self->named_types.opaque_type = cubec_context_create_type(
      self, CUBEC_VALUE_TYPE_OPAQUE, sizeof(void *), "opaque", NULL);
  self->named_types.error_type = cubec_context_create_type(
      self, CUBEC_VALUE_TYPE_ERROR, sizeof(char *), "error", NULL);
}

static void cubec_context_init_constants(cubec_context_t self) {}

cubec_context_t cubec_create_context(cubec_allocator_t allocator) {
  cubec_context_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_context_t),
                            (cubec_dispose_fn_t)cubec_context_dispose);
  self->allocator = allocator;
  self->root = cubec_create_scope(allocator, NULL);
  self->current = self->root;
  cubec_list_initialize_t strings_initialize = {
      .autofree = true,
  };
  self->strings = cubec_create_list(self->allocator, &strings_initialize);
  cubec_list_initialize_t function_initialize = {
      .autofree = true,
  };
  self->functions = cubec_create_list(self->allocator, &function_initialize);
  cubec_context_init_types(self);
  cubec_context_init_constants(self);
  return self;
}
void cubec_context_push(cubec_context_t self) {
  cubec_scope_t scope = cubec_create_scope(self->allocator, self->current);
  self->current = scope;
}
void cubec_context_pop(cubec_context_t self) {
  cubec_scope_t scope = self->current->parent;
  cubec_list_node_t it = cubec_list_get_last(self->current->defers);
  while (it != cubec_list_get_begin(self->current->defers)) {
    // TODO: eval
    it = cubec_list_node_last(it);
  }
  cubec_allocator_free(self->allocator, self->current);
  self->current = scope;
}
cubec_type_t cubec_context_create_type(cubec_context_t self,
                                       cubec_type_kind_t kind, size_t size,
                                       const char *name, void *meta) {
  cubec_type_t type =
      cubec_create_type(self->allocator, kind, size, name, meta);
  cubec_list_append(self->types, self->allocator, type);
  if (name) {
    cubec_scope_store_type(self->root, self->allocator, name, type);
  }
  return type;
}

cubec_type_t cubec_context_load_type(cubec_context_t self, const char *name) {
  cubec_scope_t scope = self->current;
  while (scope) {
    cubec_type_t type = cubec_scope_load_type(scope, name);
    if (type) {
      return type;
    }
    scope = scope->parent;
  }
  return NULL;
}
cubec_type_t cubec_context_store_type(cubec_context_t self, const char *name,
                                      cubec_type_t type) {
  cubec_scope_store_type(self->current, self->allocator, name, type);
  return type;
}

cubec_type_t cubec_context_get_ptr_type(cubec_context_t self,
                                        cubec_type_t src) {
  if (src->kind == CUBEC_VALUE_TYPE_PTR) {
    return src;
  }
  if (src->kind == CUBEC_VALUE_TYPE_REF) {
    src = src->meta;
  }
  char name[strlen(src->name) + 2];
  sprintf(name, "*%s", src->name);
  cubec_type_t ptr_type = cubec_context_load_type(self, name);
  if (!ptr_type) {
    cubec_ptr_meta_t meta = cubec_create_ptr_meta(self->allocator, src);
    ptr_type = cubec_context_create_type(self, CUBEC_VALUE_TYPE_PTR,
                                         sizeof(void *), name, meta);
  }
  return ptr_type;
}
cubec_type_t cubec_context_get_ptr_array_type(cubec_context_t self,
                                              cubec_type_t src) {
  cubec_array_meta_t meta = src->meta;
  char name[strlen(meta->type->name) + 8];
  sprintf(name, "[*]%s", src->name);
  cubec_type_t ptr_array_type = cubec_context_load_type(self, name);
  if (!ptr_array_type) {
    cubec_ptr_array_meta_t meta =
        cubec_create_ptr_array_meta(self->allocator, src);
    ptr_array_type = cubec_context_create_type(self, CUBEC_VALUE_TYPE_PTR_ARRAY,
                                               sizeof(void *), name, meta);
  }
  return ptr_array_type;
}
cubec_type_t cubec_context_get_ref_type(cubec_context_t self,
                                        cubec_type_t src) {
  if (src->kind == CUBEC_VALUE_TYPE_PTR) {
    src = src->meta;
  }
  if (src->kind == CUBEC_VALUE_TYPE_REF) {
    return src;
  }
  char name[strlen(src->name) + 2];
  sprintf(name, "&%s", src->name);
  cubec_type_t ref_type = cubec_context_load_type(self, name);
  if (!ref_type) {
    cubec_ptr_meta_t meta = cubec_create_ptr_meta(self->allocator, src);
    ref_type = cubec_context_create_type(self, CUBEC_VALUE_TYPE_REF,
                                         sizeof(void *), name, meta);
  }
  return ref_type;
}

static int cubec_type_compare(cubec_type_t a, cubec_type_t b) {
  return a->kind - b->kind;
}

cubec_type_t cubec_context_create_union_type(cubec_context_t self,
                                             const cubec_array_t src_types) {

  cubec_array_initialize_t initialize = {
      .autofree = false,
      .capacity = cubec_array_get_size(src_types),
      .compare = (cubec_compare_fn_t)cubec_type_compare,
  };
  cubec_array_t types = cubec_create_array(self->allocator, &initialize);
  cubec_array_resize(types, self->allocator, initialize.capacity);
  size_t len = 0;
  size_t size = 0;
  for (size_t idx = 0; idx < cubec_array_get_size(src_types); idx++) {
    cubec_type_t type = cubec_array_get_index(src_types, idx);
    len += strlen(type->name);
    if (type->size >= size) {
      size = type->size;
    }
    cubec_array_push(types, self->allocator, type);
  }
  cubec_array_sort(types, NULL);
  len += cubec_array_get_size(types);
  len += 8;
  char name[len];
  size_t offset = 0;
  strcpy(&name[offset], "union<");
  offset += 6;
  for (size_t idx = 0; idx < cubec_array_get_size(types); idx++) {
    if (idx != 0) {
      name[offset] = ',';
      offset++;
    }
    cubec_type_t type = cubec_array_get_index(types, idx);
    strcpy(&name[offset], type->name);
    offset += strlen(type->name);
  }
  name[offset] = '>';
  offset++;
  name[offset] = 0;
  cubec_type_t type = cubec_scope_load_type(self->root, name);
  if (!type) {
    if (size % sizeof(cubec_type_t) != 0) {
      size = size - size % sizeof(cubec_type_t) + sizeof(cubec_type_t);
    }
    size += sizeof(cubec_type_t);
    cubec_union_meta_t meta = cubec_create_union_meta(self->allocator, types);
    type = cubec_context_create_type(self, CUBEC_VALUE_TYPE_UNION, size, name,
                                     meta);
  } else {
    cubec_allocator_free(self->allocator, types);
  }
  return type;
}

cubec_type_t cubec_context_create_function_type(cubec_context_t self,
                                                size_t argc, cubec_type_t *argv,
                                                cubec_type_t type,
                                                bool variadic) {
  size_t len = 0;
  cubec_array_t args = cubec_create_array(self->allocator, NULL);
  cubec_array_resize(args, self->allocator, argc);
  for (size_t idx = 0; idx < argc; idx++) {
    if (idx != 0) {
      len++;
    }
    cubec_type_t arg = argv[idx];
    cubec_array_push(args, self->allocator, arg);
    len += strlen(arg->name);
  }
  if (variadic) {
    len += 4;
  }
  len += strlen(type->name);
  len += 16;
  char name[len];
  size_t offset = 0;
  strcpy(&name[offset], "func(");
  offset += 5;
  for (size_t idx = 0; idx < argc; idx++) {
    cubec_type_t arg = argv[idx];
    if (idx != 0) {
      name[offset] = ',';
      offset++;
    }
    strcpy(&name[offset], arg->name);
    offset += strlen(arg->name);
  }
  if (variadic) {
    strcpy(&name[offset], ",...");
    offset += 4;
  }
  name[offset++] = ')';
  name[offset++] = ':';
  strcpy(&name[offset], type->name);
  offset += strlen(type->name);
  name[offset] = 0;
  cubec_type_t func_type = cubec_context_load_type(self, name);
  if (!func_type) {
    cubec_function_meta_t meta =
        cubec_create_function_meta(self->allocator, args, type, variadic);
    func_type = cubec_context_create_type(self, CUBEC_VALUE_TYPE_FUNCTION,
                                          sizeof(void *), name, meta);
  }
  return func_type;
}

cubec_type_t cubec_context_create_array_type(cubec_context_t self,
                                             cubec_type_t type, size_t length) {
  char name[strlen(type->name) + 16];
  sprintf(name, "[%" PRIuPTR "]%s", length, type->name);
  cubec_type_t array_type = cubec_context_load_type(self, name);
  if (!array_type) {
    cubec_array_meta_t meta =
        cubec_create_array_meta(self->allocator, type, length);
    array_type = cubec_context_create_type(self, CUBEC_VALUE_TYPE_ARRAY,
                                           type->size * length, name, meta);
  }
  return array_type;
}

cubec_type_t cubec_context_create_struct_type(cubec_context_t self) {
  static size_t counter = 0;
  char name[32];
  sprintf(name, "struct@%" PRIuPTR, counter++);
  cubec_struct_meta_t meta = cubec_create_struct_meta(self->allocator);
  return cubec_context_create_type(self, CUBEC_VALUE_TYPE_STRUCT, 0, name,
                                   meta);
}

void cubec_context_add_struct_field(cubec_context_t self,
                                    cubec_type_t struct_type, const char *field,
                                    cubec_type_t type) {
  size_t offset = struct_type->size;
  size_t field_align = type->size;
  if (type->kind == CUBEC_VALUE_TYPE_STRUCT) {
    cubec_struct_meta_t meta = type->meta;
    field_align = meta->align;
  }
  if (offset % field_align != 0) {
    offset = offset - (offset % field_align) + field_align;
  }
  cubec_struct_field_desc_t desc =
      cubec_create_struct_field_desc(self->allocator, field, offset, type);
  offset += type->size;
  cubec_struct_meta_t meta = struct_type->meta;
  if (meta->align < field_align) {
    meta->align = field_align;
  }
  if (offset % meta->align != 0) {
    offset = offset - (offset % meta->align) + meta->align;
  }
  struct_type->size = offset;
  cubec_array_push(meta->fields, self->allocator, desc);
}

void cubec_context_add_struct_attribute(cubec_context_t self,
                                        cubec_type_t struct_type,
                                        const char *field,
                                        cubec_value_t value) {
  cubec_struct_meta_t meta = struct_type->meta;
  cubec_value_t val = cubec_clone_value(self->allocator, value);
  cubec_map_set(meta->attributes, self->allocator,
                cubec_create_cstring(self->allocator, field), val, NULL);
}

cubec_type_t cubec_context_create_enum_type(cubec_context_t self,
                                            cubec_type_t type) {
  static size_t counter = 0;
  char name[32];
  sprintf(name, "enum@%" PRIuPTR, counter++);
  cubec_enum_meta_t meta = cubec_create_enum_meta(self->allocator, type);
  return cubec_context_create_type(self, CUBEC_VALUE_TYPE_ENUM, type->size,
                                   NULL, meta);
}
void cubec_context_add_enum_option(cubec_context_t self, cubec_type_t enum_type,
                                   const char *name, cubec_value_t value) {
  cubec_enum_meta_t meta = enum_type->meta;
  cubec_value_t val = cubec_clone_value(self->allocator, value);
  cubec_enum_option_t opt =
      cubec_create_enum_option(self->allocator, name, val);
  cubec_array_push(meta->options, self->allocator, opt);
}
cubec_value_t cubec_context_create_enum_value(cubec_context_t self,
                                              cubec_type_t type,
                                              const char *option,
                                              const char *name) {
  cubec_enum_meta_t meta = type->meta;
  cubec_enum_option_t opt = NULL;
  for (size_t idx = 0; idx < cubec_array_get_size(meta->options); idx++) {
    cubec_enum_option_t o = cubec_array_get_index(meta->options, idx);
    if (strcmp(o->name, option) == 0) {
      opt = o;
      break;
    }
  }
  if (!opt) {
    return NULL;
  }
  return cubec_context_create_value(self, meta->type, opt->value->data, name);
}

cubec_value_t cubec_context_create_union_value(cubec_context_t self,
                                               cubec_type_t type,
                                               cubec_value_t value,
                                               const char *name) {
  uint8_t data[type->size];
  memcpy(&data[0], value->data, value->type->size);
  cubec_type_t *ptype =
      (cubec_type_t *)((uint8_t *)&data[0] + type->size - sizeof(cubec_type_t));
  *ptype = value->type;
  return cubec_context_create_value(self, type, data, name);
}

cubec_value_t cubec_context_unwrap_union(cubec_context_t self,
                                         cubec_value_t value) {
  void *data = value->data;
  cubec_type_t *ptype = (cubec_type_t *)((uint8_t *)data + value->type->size -
                                         sizeof(cubec_type_t));
  return cubec_context_create_value(self, *ptype, data, NULL);
}
cubec_value_t cubec_context_get_index(cubec_context_t self, cubec_value_t value,
                                      size_t idx) {
  cubec_array_meta_t meta = value->type->meta;
  if (idx >= meta->length) {
    return NULL;
  }
  uint8_t *data = value->data;
  return cubec_context_create_value(self, meta->type,
                                    &data[idx * meta->type->size], NULL);
}
cubec_value_t cubec_context_get_field(cubec_context_t self, cubec_value_t value,
                                      const char *field) {
  cubec_struct_meta_t meta = value->type->meta;
  cubec_struct_field_desc_t desc = NULL;
  for (size_t idx = 0; idx < cubec_array_get_size(meta->fields); idx++) {
    cubec_struct_field_desc_t f = cubec_array_get_index(meta->fields, idx);
    if (strcmp(f->name, field) == 0) {
      desc = f;
      break;
    }
  }
  if (!desc) {
    return NULL;
  }
  uint8_t *data = value->data;
  return cubec_context_create_value(self, desc->type, &data[desc->offset],
                                    NULL);
}

cubec_value_t cubec_context_create_comptime_function(cubec_context_t self,
                                                     cubec_type_t type,
                                                     cubec_ast_node_t node,
                                                     const char *name) {
  cubec_function_desc_t desc =
      cubec_create_comptime_function_desc(self->allocator, node);
  cubec_list_append(self->functions, self->allocator, desc);
  struct _cubec_function_data_t data = {
      .pfunc = desc,
  };
  return cubec_context_create_value(self, type, &data, name);
}

cubec_value_t
cubec_context_create_native_function(cubec_context_t self, cubec_type_t type,
                                     cubec_native_handle_fn_t handle,
                                     const char *name) {
  cubec_function_desc_t desc =
      cubec_create_native_function_desc(self->allocator, handle);
  cubec_list_append(self->functions, self->allocator, desc);
  struct _cubec_function_data_t data = {
      .pfunc = desc,
  };
  return cubec_context_create_value(self, type, &data, name);
}

cubec_value_t cubec_context_create_error(cubec_context_t self,
                                         const char *message,
                                         const char *name) {
  char *msg = cubec_create_cstring(self->allocator, message);
  cubec_list_append(self->strings, self->allocator, msg);
  return cubec_context_create_value(self, self->named_types.error_type, &msg,
                                    name);
}

cubec_value_t cubec_context_create_ref(cubec_context_t self, cubec_value_t src,
                                       const char *name) {
  struct _cubec_ptr_data_t data = {
      .data = src->data,
  };
  return cubec_context_create_value(
      self, cubec_context_get_ref_type(self, src->type), &data, name);
}
cubec_value_t cubec_context_create_ptr(cubec_context_t self, cubec_value_t src,
                                       const char *name) {
  struct _cubec_ptr_data_t data = {
      .data = src->data,
  };
  return cubec_context_create_value(
      self, cubec_context_get_ptr_type(self, src->type), &data, name);
}
cubec_value_t cubec_context_create_ptr_array(cubec_context_t self,
                                             cubec_value_t src,
                                             const char *name) {
  struct _cubec_ptr_data_t data = {
      .data = src->data,
  };
  return cubec_context_create_value(
      self, cubec_context_get_ptr_array_type(self, src->type), &data, name);
}
cubec_value_t cubec_context_create_value(cubec_context_t self,
                                         cubec_type_t type, const void *init,
                                         const char *name) {
  size_t size = type->size;
  if (type->kind == CUBEC_VALUE_TYPE_REF) {
    size = sizeof(void *);
  }
  void *data = cubec_allocator_alloc(self->allocator, size, NULL);
  if (init) {
    memcpy(data, init, type->size);
  } else {
    memset(data, 0, type->size);
  }
  cubec_value_t value = cubec_create_value(self->allocator, type, data);
  cubec_scope_store_value(self->current, self->allocator, value, name);
  return value;
}

cubec_value_t cubec_context_create_int8(cubec_context_t self, int8_t value,
                                        const char *name) {
  return cubec_context_create_value(self, self->named_types.int8_type, &value,
                                    name);
}
cubec_value_t cubec_context_create_int16(cubec_context_t self, int16_t value,
                                         const char *name) {
  return cubec_context_create_value(self, self->named_types.int16_type, &value,
                                    name);
}
cubec_value_t cubec_context_create_int32(cubec_context_t self, int32_t value,
                                         const char *name) {
  return cubec_context_create_value(self, self->named_types.int32_type, &value,
                                    name);
}
cubec_value_t cubec_context_create_int64(cubec_context_t self, int64_t value,
                                         const char *name) {
  return cubec_context_create_value(self, self->named_types.int64_type, &value,
                                    name);
}
cubec_value_t cubec_context_create_uint8(cubec_context_t self, uint8_t value,
                                         const char *name) {
  return cubec_context_create_value(self, self->named_types.uint8_type, &value,
                                    name);
}
cubec_value_t cubec_context_create_uint16(cubec_context_t self, uint16_t value,
                                          const char *name) {
  return cubec_context_create_value(self, self->named_types.uint16_type, &value,
                                    name);
}
cubec_value_t cubec_context_create_uint32(cubec_context_t self, uint32_t value,
                                          const char *name) {
  return cubec_context_create_value(self, self->named_types.uint32_type, &value,
                                    name);
}
cubec_value_t cubec_context_create_uint64(cubec_context_t self, uint64_t value,
                                          const char *name) {
  return cubec_context_create_value(self, self->named_types.uint64_type, &value,
                                    name);
}
cubec_value_t cubec_context_create_float32(cubec_context_t self, float value,
                                           const char *name) {
  return cubec_context_create_value(self, self->named_types.float32_type,
                                    &value, name);
}
cubec_value_t cubec_context_create_float64(cubec_context_t self, double value,
                                           const char *name) {
  return cubec_context_create_value(self, self->named_types.float64_type,
                                    &value, name);
}
cubec_value_t cubec_context_create_boolean(cubec_context_t self, bool value,
                                           const char *name) {
  return cubec_context_create_value(self, self->named_types.boolean_type,
                                    &value, name);
}
cubec_value_t cubec_context_create_undefined(cubec_context_t self,
                                             const char *name) {
  return cubec_context_create_value(self, self->named_types.undefined_type,
                                    NULL, name);
}
cubec_value_t cubec_context_create_str(cubec_context_t self, const char *value,
                                       const char *name) {
  char *str = cubec_create_cstring(self->allocator, value);
  cubec_list_append(self->strings, self->allocator, str);
  return cubec_context_create_value(self, self->named_types.str_type, &str,
                                    name);
}
cubec_value_t cubec_context_load_value(cubec_context_t self, const char *name) {
  cubec_scope_t scope = self->current;
  while (scope) {
    cubec_value_t value = cubec_scope_load_value(scope, name);
    if (value) {
      return value;
    }
    scope = scope->parent;
  }
  return cubec_context_create_undefined(self, NULL);
}

cubec_value_t cubec_context_eval(cubec_context_t self, const char *filename,
                                 const char *src) {
  char *source = NULL;
  if (!src) {
    FILE *fp = fopen(filename, "rb");
    fseek(fp, 0, SEEK_END);
    size_t len = ftell(fp);
    source = cubec_allocator_alloc(self->allocator, len + 1, NULL);
    fseek(fp, 0, SEEK_SET);
    fread(source, len, 1, fp);
    source[len] = 0;
    fclose(fp);
    src = source;
  }
  cubec_position_t begin = {
      .column = 1,
      .line = 1,
      .offset = src,
  };
  cubec_ast_node_t root =
      cubec_read_ast_program(self->allocator, &begin, src + strlen(src));
  if (root->type == CUBEC_NODE_TYPE_ERROR) {
    printf("Failed to compile: %s \n at %s:%" PRIuPTR ":%" PRIuPTR "\n",
           ((cubec_error_t)root)->message, "./main.cubec", root->loc.end.line,
           root->loc.end.column);
  } else {
    cubec_any_t ast = cubec_write_ast_node(root, self->allocator);
    char *json = cubec_any_to_json(ast, self->allocator);
    printf("%s\n", json);
    cubec_allocator_free(self->allocator, json);
    cubec_allocator_free(self->allocator, ast);
    // TODO: eval
  }
  cubec_allocator_free(self->allocator, root);
  cubec_allocator_free(self->allocator, source);
  return cubec_context_create_undefined(self, NULL);
}

cubec_value_t cubec_context_call(cubec_context_t self, cubec_value_t function,
                                 size_t argc, cubec_value_t *argv) {
  if (function->type->kind == CUBEC_VALUE_TYPE_FUNCTION) {
    cubec_function_data_t data = function->data;
    cubec_function_desc_t desc = data->pfunc;
    cubec_value_t result = NULL;
    cubec_scope_t scope = self->current;
    cubec_context_push(self);
    if (desc->kind == CUBEC_FUNCTION_NATIVE) {
      result = desc->handle(self, argc, argv);
    } else if (desc->kind == CUBEC_FUNCTION_COMPTIME) {
      // TODO: eval node
    } else {
      result = cubec_context_create_error(
          self, "Cannot call runtime function on comptime context", NULL);
    }
    cubec_list_append(scope->variables, self->allocator, result);
    cubec_list_node_t it =
        cubec_list_find(self->current->variables, result, NULL);
    result = cubec_list_node_move(it);
    cubec_context_pop(self);
    return result;
  } else {
    // TODO: call object __call__ method
    return cubec_context_create_error(self, "Variable is not callable", NULL);
  }
}