#include "engine/context.h"
#include "ast/expression.h"
#include "ast/node.h"
#include "ast/program.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/list.h"
#include "core/map.h"
#include "core/path.h"
#include "core/position.h"
#include "core/string.h"
#include "engine/array.h"
#include "engine/enum.h"
#include "engine/function.h"
#include "engine/module.h"
#include "engine/ptr.h"
#include "engine/scope.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/union.h"
#include "engine/value.h"
#include "runtime/vm.h"
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <xkeycheck.h>

static void cubec_context_dispose(cubec_context_t self,
                                  cubec_allocator_t allocator) {
  while (self->current) {
    cubec_context_pop(self);
  }
  cubec_allocator_free(allocator, self->types);
  cubec_allocator_free(allocator, self->functions);
  cubec_allocator_free(allocator, self->modules);
  cubec_allocator_free(allocator, self->strings);
}

static void cubec_context_init_types(cubec_context_t self) {
  cubec_list_initialize_t initialize = {
      .autofree = true,
  };
  self->types = cubec_create_list(self->allocator, &initialize);
  self->named_types.void_type = cubec_context_create_type(
      self, CUBEC_VALUE_TYPE_UNDEFINED, 0, "void", NULL);
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
      self, CUBEC_VALUE_TYPE_ERROR, sizeof(const char *), "error", NULL);
  self->named_types.type_type = cubec_context_create_struct_type(self, "Type");
  cubec_context_add_struct_field(self, self->named_types.type_type, "kind",
                                 self->named_types.int32_type);
  cubec_context_add_struct_field(self, self->named_types.type_type, "size",
                                 self->named_types.uint64_type);
  cubec_context_add_struct_field(self, self->named_types.type_type, "name",
                                 self->named_types.str_type);
  cubec_context_add_struct_field(self, self->named_types.type_type, "data",
                                 self->named_types.opaque_type);

  cubec_context_create_type_value(self, self->named_types.void_type, "void");
  cubec_context_create_type_value(self, self->named_types.type_type, "Type");
  cubec_context_create_type_value(self, self->named_types.int8_type, "int8");
  cubec_context_create_type_value(self, self->named_types.int16_type, "int16");
  cubec_context_create_type_value(self, self->named_types.int32_type, "int32");
  cubec_context_create_type_value(self, self->named_types.int64_type, "int64");
  cubec_context_create_type_value(self, self->named_types.uint8_type, "uint8");
  cubec_context_create_type_value(self, self->named_types.uint16_type,
                                  "uint16");
  cubec_context_create_type_value(self, self->named_types.uint32_type,
                                  "uint32");
  cubec_context_create_type_value(self, self->named_types.uint64_type,
                                  "uint64");
  cubec_context_create_type_value(self, self->named_types.float32_type,
                                  "float32");
  cubec_context_create_type_value(self, self->named_types.float64_type,
                                  "float64");
  cubec_context_create_type_value(self, self->named_types.boolean_type,
                                  "boolean");
  cubec_context_create_type_value(self, self->named_types.str_type, "str");
  cubec_context_create_type_value(self, self->named_types.opaque_type,
                                  "opaque");
  cubec_context_create_type_value(self, self->named_types.error_type, "error");
}

static void cubec_context_init_constants(cubec_context_t self) {
  self->constants.undefined = cubec_context_create_value(
      self, self->named_types.void_type, NULL, "undefined");
}

cubec_context_t cubec_create_context(cubec_allocator_t allocator) {
  cubec_context_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_context_t),
                            (cubec_dispose_fn_t)cubec_context_dispose);
  self->allocator = allocator;
  self->root = cubec_create_scope(allocator, NULL);
  self->current = self->root;
  cubec_list_initialize_t function_initialize = {
      .autofree = true,
  };
  self->functions = cubec_create_list(self->allocator, &function_initialize);
  cubec_list_initialize_t string_initialize = {
      .autofree = true,
  };
  self->strings = cubec_create_list(self->allocator, &string_initialize);
  cubec_map_initialize_t module_initialize = {
      .autofree_key = true,
      .autofree_value = true,
      .compare = (cubec_compare_fn_t)strcmp,
  };
  self->modules = cubec_create_map(allocator, &module_initialize);
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
  it = cubec_list_get_first(self->current->variables);
  while (it != cubec_list_get_end(self->current->variables)) {
    cubec_value_t value = cubec_list_node_get(it);
    if (value->type->kind == CUBEC_VALUE_TYPE_STRUCT) {
      cubec_struct_meta_t meta = value->type->meta;
      cubec_value_t dispose =
          cubec_map_get(meta->attributes, "__dispose__", NULL);
      if (dispose && dispose->type->kind == CUBEC_VALUE_TYPE_FUNCTION) {
        cubec_context_call(self, dispose, 1, &value);
      }
    }
    it = cubec_list_node_next(it);
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
  return type;
}

cubec_type_t cubec_context_load_type(cubec_context_t self, const char *name) {
  for (cubec_list_node_t it = cubec_list_get_first(self->types);
       it != cubec_list_get_end(self->types); it = cubec_list_node_next(it)) {
    cubec_type_t type = cubec_list_node_get(it);
    if (type->name && strcmp(type->name, name) == 0) {
      return type;
    }
  }
  return NULL;
}

cubec_type_t cubec_context_get_ptr_type(cubec_context_t self,
                                        cubec_type_t src) {
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
    cubec_ptr_meta_t meta = cubec_create_ptr_meta(self->allocator, src);
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
    strcpy(&name[offset], "...");
    offset += 3;
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

cubec_type_t cubec_context_create_struct_type(cubec_context_t self,
                                              const char *name) {
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
  cubec_struct_field_t desc =
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
                                            cubec_type_t type,
                                            const char *name) {
  cubec_enum_meta_t meta = cubec_create_enum_meta(self->allocator, type);
  return cubec_context_create_type(self, CUBEC_VALUE_TYPE_ENUM, type->size,
                                   name, meta);
}
void cubec_context_add_enum_option(cubec_context_t self, cubec_type_t enum_type,
                                   const char *name, cubec_value_t value) {
  cubec_enum_meta_t meta = enum_type->meta;
  cubec_value_t val = cubec_clone_value(self->allocator, value);
  cubec_map_set(meta->options, self->allocator,
                cubec_create_cstring(self->allocator, name), val, NULL);
}

cubec_value_t cubec_context_resolve_enum(cubec_context_t self,
                                         cubec_value_t value,
                                         const char *name) {
  cubec_enum_meta_t meta = value->type->meta;
  return cubec_context_create_value(self, meta->type, value->data, NULL);
}

cubec_value_t cubec_context_create_type_value(cubec_context_t self,
                                              cubec_type_t type,
                                              const char *name) {

  cubec_value_t val =
      cubec_context_create_value(self, self->named_types.type_type, NULL, name);
  cubec_value_t kind = cubec_context_create_int32(self, type->kind, NULL);
  cubec_value_t data = cubec_context_create_opaque(self, type, NULL);
  cubec_value_t str_name = cubec_context_create_str(self, type->name, NULL);
  cubec_value_t size = cubec_context_create_uint64(self, type->size, NULL);
  cubec_context_set_field(self, val, "kind", kind);
  cubec_context_set_field(self, val, "name", str_name);
  cubec_context_set_field(self, val, "size", size);
  cubec_context_set_field(self, val, "data", data);
  return val;
}
cubec_value_t cubec_context_create_enum_value(cubec_context_t self,
                                              cubec_type_t type,
                                              const char *option,
                                              const char *name) {
  cubec_enum_meta_t meta = type->meta;
  cubec_value_t value = cubec_map_get(meta->options, option, NULL);
  if (!value) {
    char name[strlen(option) + 32];
    sprintf(name, "Unknown option '%s' in enum", option);
    return cubec_context_create_error(self, name, NULL);
  }
  return cubec_context_create_value(self, type, value->data, name);
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
  cubec_struct_field_t desc = NULL;
  for (size_t idx = 0; idx < cubec_array_get_size(meta->fields); idx++) {
    cubec_struct_field_t f = cubec_array_get_index(meta->fields, idx);
    if (strcmp(f->name, field) == 0) {
      desc = f;
      break;
    }
  }
  if (!desc) {
    char msg[strlen(field) + 32];
    sprintf(msg, "Field '%s' is not in struct", field);
    return cubec_context_create_error(self, msg, NULL);
  }
  uint8_t *data = value->data;
  return cubec_context_create_value(self, desc->type, &data[desc->offset],
                                    NULL);
}

cubec_value_t cubec_context_set_field(cubec_context_t self, cubec_value_t stru,
                                      const char *field, cubec_value_t value) {
  cubec_struct_meta_t meta = stru->type->meta;
  cubec_struct_field_t desc = NULL;
  for (size_t idx = 0; idx < cubec_array_get_size(meta->fields); idx++) {
    cubec_struct_field_t f = cubec_array_get_index(meta->fields, idx);
    if (strcmp(f->name, field) == 0) {
      desc = f;
      break;
    }
  }
  if (!desc) {
    char msg[strlen(field) + 32];
    sprintf(msg, "Field '%s' is not in struct", field);
    return cubec_context_create_error(self, msg, NULL);
  }
  if (!cubec_context_is_type_equal(self, desc->type, value->type)) {
    char msg[strlen(field) + 32];
    sprintf(msg, "Field '%s' is not in struct", field);
    return cubec_context_create_error(self, msg, NULL);
  }
  memcpy(stru->data + desc->offset, value->data, value->type->size);
  return cubec_context_create_boolean(self, true, NULL);
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
  char *str = cubec_create_cstring(self->allocator, message);
  cubec_list_append(self->strings, self->allocator, str);
  return cubec_context_create_value(self, self->named_types.error_type, &str,
                                    name);
}

cubec_value_t cubec_context_create_ref(cubec_context_t self, cubec_value_t src,
                                       const char *name) {
  return cubec_context_create_value(
      self, cubec_context_get_ref_type(self, src->type), &src->data, name);
}
cubec_value_t cubec_context_create_ptr(cubec_context_t self, cubec_value_t src,
                                       const char *name) {
  return cubec_context_create_value(
      self, cubec_context_get_ptr_type(self, src->type), &src->data, name);
}
cubec_value_t cubec_context_create_ptr_array(cubec_context_t self,
                                             cubec_value_t src,
                                             const char *name) {
  return cubec_context_create_value(
      self, cubec_context_get_ptr_array_type(self, src->type), &src->data,
      name);
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
cubec_value_t cubec_context_get_undefined(cubec_context_t self) {
  return self->constants.undefined;
}

cubec_value_t cubec_context_create_str(cubec_context_t self, const char *value,
                                       const char *name) {
  char *str = cubec_create_cstring(self->allocator, value);
  cubec_list_append(self->strings, self->allocator, str);
  return cubec_context_create_value(self, self->named_types.str_type, &str,
                                    name);
}

cubec_value_t cubec_context_create_opaque(cubec_context_t self,
                                          const void *value, const char *name) {
  return cubec_context_create_value(self, self->named_types.str_type, &value,
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
  char msg[strlen(name) + 32];
  sprintf(msg, "Use of undeclared identifier '%s'", name);
  return cubec_context_create_error(self, msg, NULL);
}

cubec_value_t cubec_context_eval(cubec_context_t self, const char *filename,
                                 char *src, cubec_eval_type_t type) {
  if (type == CUBEC_EVAL_INLINE) {
    cubec_position_t position = {
        .column = 0,
        .line = 0,
        .offset = src,
    };
    cubec_ast_node_t node = cubec_read_ast_expression(
        self->allocator, &position, src + strlen(src));
    cubec_vm_t vm = cubec_create_vm(self->allocator);
    cubec_value_t value = cubec_vm_run(vm, self, node);
    cubec_allocator_free(self->allocator, vm);
    return value;
  } else if (type == CUBEC_EVAL_MODULE) {
    cubec_path_t path = cubec_create_path(self->allocator, filename);
    path = cubec_path_absolute(path, self->allocator);
    char *fullpath = cubec_path_to_string(path, self->allocator);
    cubec_path_t parent = cubec_path_parent(path, self->allocator);
    cubec_allocator_free(self->allocator, path);
    char *dirname = cubec_path_to_string(parent, self->allocator);
    cubec_allocator_free(self->allocator, parent);
    cubec_module_t module = cubec_map_get(self->modules, fullpath, NULL);
    cubec_value_t result = NULL;
    if (!module) {
      char *source = NULL;
      if (!src) {
        FILE *fp = fopen(fullpath, "rb");
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
      cubec_ast_node_t node =
          cubec_read_ast_program(self->allocator, &begin, src + strlen(src));
      module =
          cubec_create_module(self->allocator, dirname, filename, source, node);
      cubec_map_set(self->modules, self->allocator,
                    cubec_create_cstring(self->allocator, fullpath), module,
                    NULL);
      cubec_module_t current_module = self->module;
      self->module = module;
      cubec_vm_t vm = cubec_create_vm(self->allocator);
      result = cubec_vm_run(vm, self, node);
      cubec_allocator_free(self->allocator, vm);
    }
    cubec_allocator_free(self->allocator, fullpath);
    cubec_allocator_free(self->allocator, dirname);
    if (!result) {
      result = cubec_context_get_undefined(self);
    }
    return result;
  }
  return self->constants.undefined;
}

bool cubec_context_is_type_equal(cubec_context_t self, cubec_type_t dst,
                                 cubec_type_t src) {
  if (dst->kind != src->kind) {
    return false;
  }
  if (dst->kind == CUBEC_VALUE_TYPE_PTR ||
      dst->kind == CUBEC_VALUE_TYPE_PTR_ARRAY) {
    cubec_ptr_meta_t dst_meta = dst->meta;
    cubec_ptr_meta_t src_meta = src->meta;
    return cubec_context_is_type_equal(self, dst_meta->type, src_meta->type);
  }
  if (dst->kind == CUBEC_VALUE_TYPE_ARRAY) {
    cubec_array_meta_t dst_meta = dst->meta;
    cubec_array_meta_t src_meta = src->meta;
    if (dst_meta->length != src_meta->length) {
      return false;
    }
    return cubec_context_is_type_equal(self, dst_meta->type, src_meta->type);
  }
  if (dst->kind == CUBEC_VALUE_TYPE_STRUCT) {
    cubec_struct_meta_t dst_meta = dst->meta;
    cubec_struct_meta_t src_meta = src->meta;
    if (dst_meta->align != src_meta->align) {
      return false;
    }
    for (size_t idx = 0; idx < cubec_array_get_size(dst_meta->fields); idx++) {
      cubec_struct_field_t dst_field =
          cubec_array_get_index(dst_meta->fields, idx);
      cubec_struct_field_t src_field =
          cubec_array_get_index(src_meta->fields, idx);
      if (dst_field->offset != src_field->offset) {
        return false;
      }
      if (strcmp(dst_field->name, src_field->name) != 0) {
        return false;
      }
      if (!cubec_context_is_type_equal(self, dst_field->type,
                                       src_field->type)) {
        return false;
      }
    }
  }
  if (dst->kind == CUBEC_VALUE_TYPE_FUNCTION) {
    cubec_function_meta_t dst_meta = dst->meta;
    cubec_function_meta_t src_meta = src->meta;
    if (!cubec_context_is_type_equal(self, dst_meta->type, src_meta->type)) {
      return false;
    }
    if (dst_meta->variadic && !src_meta->variadic) {
      return false;
    }
    if (!dst_meta->variadic) {
      if (!src_meta->variadic && cubec_array_get_size(dst_meta->args) >
                                     cubec_array_get_size(src_meta->args)) {
        return false;
      }
    }
    for (size_t idx = 0; idx < cubec_array_get_size(dst_meta->args); idx++) {
      if (idx >= cubec_array_get_size(src_meta->args)) {
        break;
      }
      cubec_type_t dst_arg = cubec_array_get_index(dst_meta->args, idx);
      cubec_type_t src_arg = cubec_array_get_index(src_meta->args, idx);
      if (!cubec_context_is_type_equal(self, dst_arg, src_arg)) {
        return false;
      }
    }
  }
  if (dst->kind == CUBEC_VALUE_TYPE_ENUM) {
    cubec_enum_meta_t dst_meta = dst->meta;
    cubec_enum_meta_t src_meta = src->meta;
    if (!cubec_context_is_type_equal(self, dst_meta->type, src_meta->type)) {
      return false;
    }
    for (cubec_list_node_t it = cubec_map_get_first(dst_meta->options);
         it != cubec_map_get_end(dst_meta->options);
         it = cubec_map_node_get_next(it)) {
      const char *key = cubec_map_node_get_key(it);
      cubec_value_t value = cubec_map_node_get_value(it);
      cubec_value_t src_value = cubec_map_get(src_meta->options, key, NULL);
      if (!src_value) {
        return false;
      }
      if (memcmp(value->data, src_value->data, dst_meta->type->size) != 0) {
        return false;
      }
    }
  }
  return true;
}
cubec_value_t cubec_context_convert(cubec_context_t self, cubec_type_t dst,
                                    cubec_value_t value) {
  if (value->type->kind == CUBEC_VALUE_TYPE_REF) {
    cubec_value_t *pval = value->data;
    return cubec_context_convert(self, dst, *pval);
  }
  cubec_type_t src = value->type;
  if (cubec_context_is_type_equal(self, dst, src)) {
    return cubec_context_create_value(self, dst, value->data, NULL);
  }
  if (src->kind >= CUBEC_VALUE_TYPE_INT8 &&
      src->kind <= CUBEC_VALUE_TYPE_INT64) {
    int64_t val = 0;
    if (src->kind == CUBEC_VALUE_TYPE_INT8) {
      val = *(int8_t *)value->data;
    } else if (src->kind == CUBEC_VALUE_TYPE_INT16) {
      val = *(int16_t *)value->data;
    } else if (src->kind == CUBEC_VALUE_TYPE_INT32) {
      val = *(int32_t *)value->data;
    } else if (src->kind == CUBEC_VALUE_TYPE_INT64) {
      val = *(int64_t *)value->data;
    }
    if (dst->kind == CUBEC_VALUE_TYPE_INT8) {
      if (val > INT8_MAX || val < INT8_MIN) {
        char msg[128];
        sprintf(msg, "Implicit conversion from '%" PRIdPTR "' to 'int8'", val);
        return cubec_context_create_error(self, msg, NULL);
      }
      return cubec_context_create_int8(self, (int8_t)val, NULL);
    } else if (dst->kind == CUBEC_VALUE_TYPE_INT16) {
      if (val > INT16_MAX || val < INT16_MIN) {
        char msg[128];
        sprintf(msg, "Implicit conversion from '%" PRIdPTR "' to 'int16'", val);
        return cubec_context_create_error(self, msg, NULL);
      }
      return cubec_context_create_int16(self, (int8_t)val, NULL);
    } else if (dst->kind == CUBEC_VALUE_TYPE_INT32) {
      if (val > INT32_MAX || val < INT32_MIN) {
        char msg[128];
        sprintf(msg, "Implicit conversion from '%" PRIdPTR "' to 'int32'", val);
        return cubec_context_create_error(self, msg, NULL);
      }
      return cubec_context_create_int32(self, (int8_t)val, NULL);
    } else if (dst->kind == CUBEC_VALUE_TYPE_INT64) {
      return cubec_context_create_int64(self, val, NULL);
    } else if (dst->kind == CUBEC_VALUE_TYPE_UINT8) {
      if (val > UINT8_MAX || val < 0) {
        char msg[128];
        sprintf(msg, "Implicit conversion from '%" PRIdPTR "' to 'uint8'", val);
        return cubec_context_create_error(self, msg, NULL);
      }
      return cubec_context_create_uint8(self, (int8_t)val, NULL);
    } else if (dst->kind == CUBEC_VALUE_TYPE_UINT16) {
      if (val > UINT16_MAX || val < 0) {
        char msg[128];
        sprintf(msg, "Implicit conversion from '%" PRIdPTR "' to 'uint16'",
                val);
        return cubec_context_create_error(self, msg, NULL);
      }
      return cubec_context_create_uint16(self, (int8_t)val, NULL);
    } else if (dst->kind == CUBEC_VALUE_TYPE_UINT32) {
      if (val > UINT32_MAX || val < 0) {
        char msg[128];
        sprintf(msg, "Implicit conversion from '%" PRIdPTR "' to 'uint32'",
                val);
        return cubec_context_create_error(self, msg, NULL);
      }
      return cubec_context_create_uint32(self, (int8_t)val, NULL);
    } else if (dst->kind == CUBEC_VALUE_TYPE_UINT64) {
      if (val < 0) {
        char msg[128];
        sprintf(msg, "Implicit conversion from '%" PRIdPTR "' to 'uint64'",
                val);
        return cubec_context_create_error(self, msg, NULL);
      }
      return cubec_context_create_uint64(self, val, NULL);
    } else if (dst->kind == CUBEC_VALUE_TYPE_FLOAT32) {
      return cubec_context_create_float32(self, val, NULL);
    } else if (dst->kind == CUBEC_VALUE_TYPE_FLOAT64) {
      return cubec_context_create_float64(self, val, NULL);
    }
  }

  if (src->kind >= CUBEC_VALUE_TYPE_UINT8 &&
      src->kind <= CUBEC_VALUE_TYPE_UINT64) {
    uint64_t val = 0;
    if (src->kind == CUBEC_VALUE_TYPE_UINT8) {
      val = *(uint8_t *)value->data;
    } else if (src->kind == CUBEC_VALUE_TYPE_UINT16) {
      val = *(uint16_t *)value->data;
    } else if (src->kind == CUBEC_VALUE_TYPE_UINT32) {
      val = *(uint32_t *)value->data;
    } else if (src->kind == CUBEC_VALUE_TYPE_UINT64) {
      val = *(uint64_t *)value->data;
    }
    if (dst->kind == CUBEC_VALUE_TYPE_INT8) {
      if (val > INT8_MAX) {
        char msg[128];
        sprintf(msg, "Implicit conversion from '%" PRIdPTR "' to 'int8'", val);
        return cubec_context_create_error(self, msg, NULL);
      }
      return cubec_context_create_int8(self, (int8_t)val, NULL);
    } else if (dst->kind == CUBEC_VALUE_TYPE_INT16) {
      if (val > INT16_MAX) {
        char msg[128];
        sprintf(msg, "Implicit conversion from '%" PRIdPTR "' to 'int16'", val);
        return cubec_context_create_error(self, msg, NULL);
      }
      return cubec_context_create_int16(self, (int8_t)val, NULL);
    } else if (dst->kind == CUBEC_VALUE_TYPE_INT32) {
      if (val > INT32_MAX) {
        char msg[128];
        sprintf(msg, "Implicit conversion from '%" PRIdPTR "' to 'int32'", val);
        return cubec_context_create_error(self, msg, NULL);
      }
      return cubec_context_create_int32(self, (int8_t)val, NULL);
    } else if (dst->kind == CUBEC_VALUE_TYPE_INT64) {
      return cubec_context_create_int64(self, val, NULL);
    } else if (dst->kind == CUBEC_VALUE_TYPE_UINT8) {
      if (val > UINT8_MAX) {
        char msg[128];
        sprintf(msg, "Implicit conversion from '%" PRIdPTR "' to 'uint8'", val);
        return cubec_context_create_error(self, msg, NULL);
      }
      return cubec_context_create_uint8(self, (int8_t)val, NULL);
    } else if (dst->kind == CUBEC_VALUE_TYPE_UINT16) {
      if (val > UINT16_MAX) {
        char msg[128];
        sprintf(msg, "Implicit conversion from '%" PRIdPTR "' to 'uint16'",
                val);
        return cubec_context_create_error(self, msg, NULL);
      }
      return cubec_context_create_uint16(self, (int8_t)val, NULL);
    } else if (dst->kind == CUBEC_VALUE_TYPE_UINT32) {
      if (val > UINT32_MAX) {
        char msg[128];
        sprintf(msg, "Implicit conversion from '%" PRIdPTR "' to 'uint32'",
                val);
        return cubec_context_create_error(self, msg, NULL);
      }
      return cubec_context_create_uint32(self, (int8_t)val, NULL);
    } else if (dst->kind == CUBEC_VALUE_TYPE_UINT64) {
      return cubec_context_create_uint64(self, val, NULL);
    } else if (dst->kind == CUBEC_VALUE_TYPE_FLOAT32) {
      return cubec_context_create_float32(self, val, NULL);
    } else if (dst->kind == CUBEC_VALUE_TYPE_FLOAT64) {
      return cubec_context_create_float64(self, val, NULL);
    }
  }
  if (src->kind == CUBEC_VALUE_TYPE_FLOAT32 ||
      src->kind == CUBEC_VALUE_TYPE_FLOAT64) {
    double val = *(double *)value->data;
    if (dst->kind == CUBEC_VALUE_TYPE_FLOAT32) {
      return cubec_context_create_float32(self, val, NULL);
    } else if (dst->kind == CUBEC_VALUE_TYPE_FLOAT64) {
      return cubec_context_create_float64(self, val, NULL);
    }
  }
  if (dst->kind == CUBEC_VALUE_TYPE_REF) {
    cubec_ptr_meta_t meta = dst->meta;
    if (cubec_context_is_type_equal(self, meta->type, src)) {
      return cubec_context_create_ref(self, value, NULL);
    }
  }
  if (dst->kind == CUBEC_VALUE_TYPE_PTR && src->kind == CUBEC_VALUE_TYPE_PTR) {
    cubec_ptr_meta_t dst_ptr_meta = dst->meta;
    cubec_ptr_meta_t src_ptr_meta = src->meta;
    if (dst_ptr_meta->type->kind == CUBEC_VALUE_TYPE_STRUCT &&
        src_ptr_meta->type->kind == CUBEC_VALUE_TYPE_STRUCT) {
      cubec_struct_meta_t dst_meta = dst_ptr_meta->type->meta;
      cubec_struct_meta_t src_meta = src_ptr_meta->type->meta;
      if (dst_meta->align == src_meta->align) {
        cubec_array_t dst_fields =
            cubec_flat_struct_fields(self->allocator, dst_meta->fields);
        cubec_array_t src_fields =
            cubec_flat_struct_fields(self->allocator, src_meta->fields);
        size_t idx = 0;
        size_t num_fields = cubec_array_get_size(dst_fields);
        for (idx = 0; idx < num_fields; idx++) {
          cubec_struct_field_t dst_field =
              cubec_array_get_index(dst_fields, idx);
          cubec_struct_field_t src_field =
              cubec_array_get_index(src_fields, idx);
          if (dst_field->offset != src_field->offset) {
            break;
          }
          if (!cubec_context_is_type_equal(self, dst_field->type,
                                           src_field->type)) {
            break;
          }
        }
        cubec_allocator_free(self->allocator, dst_fields);
        cubec_allocator_free(self->allocator, src_fields);
        if (idx == num_fields) {
          return cubec_context_create_value(self, dst, value->data, NULL);
        }
      }
    }
  }
  if (dst->kind == CUBEC_VALUE_TYPE_PTR_ARRAY &&
      src->kind == CUBEC_VALUE_TYPE_ARRAY) {
    cubec_ptr_meta_t dst_meta = dst->meta;
    cubec_array_meta_t src_meta = src->meta;
    if (cubec_context_is_type_equal(self, dst_meta->type, src_meta->type)) {
      return cubec_context_create_ptr_array(self, value, NULL);
    }
  }
  char msg[strlen(value->type->name) + strlen(dst->name) + 128];
  sprintf(msg, "Cannot convert %s to %s", value->type->name, dst->name);
  return cubec_context_create_error(self, msg, NULL);
}

cubec_value_t cubec_context_call(cubec_context_t self, cubec_value_t function,
                                 size_t argc, cubec_value_t *argv) {
  if (function->type->kind == CUBEC_VALUE_TYPE_FUNCTION) {
    cubec_function_meta_t meta = function->type->meta;
    cubec_function_data_t data = function->data;
    cubec_function_desc_t desc = data->pfunc;
    cubec_value_t result =
        cubec_context_create_value(self, meta->type, NULL, NULL);
    cubec_context_push(self);
    cubec_value_t args[argc];
    for (size_t idx = 0; idx < argc; idx++) {
      cubec_value_t arg = NULL;
      if (idx < cubec_array_get_size(meta->args)) {
        arg = cubec_context_convert(
            self, cubec_array_get_index(meta->args, idx), argv[idx]);
      } else if (meta->variadic) {
        arg = cubec_context_create_value(self, argv[idx]->type, argv[idx]->data,
                                         NULL);
      } else {
        arg = cubec_context_create_error(
            self, "Too many args for call function", NULL);
      }
      if (arg->type->kind == CUBEC_VALUE_TYPE_ERROR) {
        return arg;
      }
      args[idx] = arg;
    }
    cubec_value_t res = NULL;
    if (desc->kind == CUBEC_FUNCTION_NATIVE) {
      res = desc->handle(self, argc, args);
    } else if (desc->kind == CUBEC_FUNCTION_COMPTIME) {
      // TODO: eval
    } else {
      res = cubec_context_create_error(
          self, "Cannot call runtime function on comptime context", NULL);
    }
    if (res->type->kind == CUBEC_VALUE_TYPE_ERROR) {
      result->type = res->type;
      cubec_allocator_free(self->allocator, result->data);
      result->data =
          cubec_allocator_alloc(self->allocator, result->type->size, NULL);
      memcpy(result->data, res->data, result->type->size);
    } else {
      memcpy(result->data, res->data, result->type->size);
    }
    cubec_context_pop(self);
    return result;
  } else if (function->type->kind == CUBEC_VALUE_TYPE_REF) {
    cubec_value_t *pvalue = function->data;
    return cubec_context_call(self, *pvalue, argc, argv);
  } else {
    if (function->type->kind == CUBEC_VALUE_TYPE_STRUCT) {
      cubec_struct_meta_t meta = function->type->meta;
      cubec_value_t callee = cubec_map_get(meta->attributes, "__call__", NULL);
      if (callee && callee->type->kind == CUBEC_VALUE_TYPE_FUNCTION) {
        cubec_value_t args[argc + 1];
        for (size_t idx = 0; idx < argc; idx++) {
          args[idx + 1] = argv[idx];
        }
        cubec_function_meta_t meta = callee->type->meta;
        cubec_type_t self_type = cubec_array_get_index(meta->args, 0);
        if (self_type->kind == CUBEC_VALUE_TYPE_PTR) {
          args[0] = cubec_context_create_ptr(self, function, NULL);
        } else if (self_type->kind == CUBEC_VALUE_TYPE_REF) {
          args[0] = cubec_context_create_ref(self, function, NULL);
        } else {
          args[0] = function;
        }
        return cubec_context_call(self, callee, argc + 1, args);
      }
    }
    return cubec_context_create_error(self, "Value is not callable", NULL);
  }
}
cubec_value_t cubec_context_to_str(cubec_context_t self, cubec_value_t value) {
  switch (value->type->kind) {
  case CUBEC_VALUE_TYPE_ERROR: {
    const char **data = value->data;
    char msg[strlen(*data) + 16];
    sprintf(msg, "error: %s", *data);
    return cubec_context_create_str(self, msg, NULL);
  }
  case CUBEC_VALUE_TYPE_UNDEFINED: {
    return cubec_context_create_str(self, "undefined", NULL);
  }
  case CUBEC_VALUE_TYPE_INT8: {
    int8_t val = *(int8_t *)value->data;
    char msg[8];
    sprintf(msg, "%d", val);
    return cubec_context_create_str(self, msg, NULL);
  }
  case CUBEC_VALUE_TYPE_INT16: {
    int16_t val = *(int16_t *)value->data;
    char msg[8];
    sprintf(msg, "%d", val);
    return cubec_context_create_str(self, msg, NULL);
  }
  case CUBEC_VALUE_TYPE_INT32: {
    int32_t val = *(int32_t *)value->data;
    char msg[16];
    sprintf(msg, "%d", val);
    return cubec_context_create_str(self, msg, NULL);
  }
  case CUBEC_VALUE_TYPE_INT64: {
    int64_t val = *(int64_t *)value->data;
    char msg[16];
    sprintf(msg, "%" PRIdPTR, val);
    return cubec_context_create_str(self, msg, NULL);
  }
  case CUBEC_VALUE_TYPE_UINT8: {
    uint8_t val = *(uint8_t *)value->data;
    char msg[16];
    sprintf(msg, "%u", val);
    return cubec_context_create_str(self, msg, NULL);
  }
  case CUBEC_VALUE_TYPE_UINT16: {
    uint16_t val = *(uint16_t *)value->data;
    char msg[16];
    sprintf(msg, "%u", val);
    return cubec_context_create_str(self, msg, NULL);
  }
  case CUBEC_VALUE_TYPE_UINT32: {
    uint32_t val = *(uint32_t *)value->data;
    char msg[16];
    sprintf(msg, "%u", val);
    return cubec_context_create_str(self, msg, NULL);
  }
  case CUBEC_VALUE_TYPE_UINT64: {
    uint64_t val = *(uint64_t *)value->data;
    char msg[16];
    sprintf(msg, "%" PRIuPTR, val);
    return cubec_context_create_str(self, msg, NULL);
  }
  case CUBEC_VALUE_TYPE_FLOAT32: {
    float val = *(float *)value->data;
    char msg[32];
    sprintf(msg, "%g", val);
    return cubec_context_create_str(self, msg, NULL);
  }
  case CUBEC_VALUE_TYPE_FLOAT64: {
    double val = *(double *)value->data;
    char msg[32];
    sprintf(msg, "%g", val);
    return cubec_context_create_str(self, msg, NULL);
  }
  case CUBEC_VALUE_TYPE_BOOLEAN: {
    bool val = *(bool *)value->data;
    return cubec_context_create_str(self, val ? "true" : "false", NULL);
  }
  case CUBEC_VALUE_TYPE_STR: {
    const char **data = value->data;
    char msg[strlen(*data) * 2 + 3];
    char *dst = &msg[0];
    *dst++ = '\"';
    const char *src = *data;
    while (*src) {
      if (*src == '\n') {
        *dst++ = '\\';
        *dst++ = 'n';
      } else if (*src == '\r') {
        *dst++ = '\\';
        *dst++ = 'r';
      } else if (*src == '\t') {
        *dst++ = '\\';
        *dst++ = 't';
      } else if (*src == '\f') {
        *dst++ = '\\';
        *dst++ = 'f';
      } else if (*src == '\a') {
        *dst++ = '\\';
        *dst++ = 'a';
      } else if (*src == '\b') {
        *dst++ = '\\';
        *dst++ = 'b';
      } else if (*src == '\?') {
        *dst++ = '\\';
        *dst++ = '?';
      } else if (*src == '\'') {
        *dst++ = '\\';
        *dst++ = '\'';
      } else if (*src == '\"') {
        *dst++ = '\\';
        *dst++ = '\"';
      } else {
        *dst++ = *src;
      }
      src++;
    }
    *dst++ = '\"';
    *dst = 0;
    return cubec_context_create_str(self, msg, NULL);
  }
  case CUBEC_VALUE_TYPE_OPAQUE: {
    intptr_t opaque = (intptr_t)value->data;
    char msg[64];
    sprintf(msg, "opaque(0x%" PRIxPTR ")", opaque);
    return cubec_context_create_str(self, msg, NULL);
  }
  case CUBEC_VALUE_TYPE_PTR: {
    cubec_ptr_meta_t meta = value->type->meta;
    intptr_t address = (intptr_t)value->data;
    char msg[64 + strlen(meta->type->name)];
    sprintf(msg, "*%s(0x%" PRIxPTR ")", meta->type->name, address);
    return cubec_context_create_str(self, msg, NULL);
  }
  case CUBEC_VALUE_TYPE_PTR_ARRAY: {
    cubec_ptr_meta_t meta = value->type->meta;
    intptr_t address = (intptr_t)value->data;
    char msg[64 + strlen(meta->type->name)];
    sprintf(msg, "[*]%s(0x%" PRIxPTR ")", meta->type->name, address);
    return cubec_context_create_str(self, msg, NULL);
  }
  case CUBEC_VALUE_TYPE_REF: {
    cubec_ptr_meta_t meta = value->type->meta;
    intptr_t address = (intptr_t)value->data;
    char msg[64 + strlen(meta->type->name)];
    sprintf(msg, "&%s(0x%" PRIxPTR ")", meta->type->name, address);
    return cubec_context_create_str(self, msg, NULL);
  }
  case CUBEC_VALUE_TYPE_STRUCT: {
    cubec_struct_meta_t meta = value->type->meta;
    size_t len = 0;
    cubec_array_initialize_t initialize = {
        .autofree = true,
    };
    cubec_array_t fields = cubec_create_array(self->allocator, &initialize);
    for (size_t idx = 0; idx < cubec_array_get_size(meta->fields); idx++) {
      cubec_struct_field_t field = cubec_array_get_index(meta->fields, idx);
      cubec_value_t val = cubec_context_get_field(self, value, field->name);
      if (val->type->kind == CUBEC_VALUE_TYPE_ERROR) {
        cubec_allocator_free(self->allocator, fields);
        return val;
      }
      val = cubec_context_to_str(self, val);
      if (val->type->kind == CUBEC_VALUE_TYPE_ERROR) {
        cubec_allocator_free(self->allocator, fields);
        return val;
      }
      const char **v = val->data;
      char *str = cubec_allocator_alloc(
          self->allocator, strlen(*v) + strlen(field->name) + 8, NULL);
      sprintf(str, "%s: %s", field->name, *v);
      len += strlen(str);
      cubec_array_push(fields, self->allocator, str);
    }
    if (value->type->name) {
      len += strlen(value->type->name);
    } else {
      len += 6;
    }
    char str[len + 32];
    size_t offset = 0;
    if (value->type->name) {
      strcpy(&str[offset], value->type->name);
      offset += strlen(value->type->name);
    } else {
      strcpy(&str[offset], "struct");
      offset += 6;
    }
    str[offset++] = '{';
    str[offset++] = ' ';
    for (size_t idx = 0; idx < cubec_array_get_size(fields); idx++) {
      if (idx != 0) {
        str[offset++] = ',';
        str[offset++] = ' ';
      }
      const char *s = cubec_array_get_index(fields, idx);
      strcpy(&str[offset], s);
      offset += strlen(s);
    }
    str[offset++] = '}';
    str[offset] = 0;
    cubec_allocator_free(self->allocator, fields);
    return cubec_context_create_str(self, str, NULL);
  }
  case CUBEC_VALUE_TYPE_ARRAY: {
    cubec_array_meta_t meta = value->type->meta;
    size_t len = 0;
    cubec_array_t items = cubec_create_array(self->allocator, NULL);
    for (size_t idx = 0; idx < meta->length; idx++) {
      cubec_value_t val = cubec_context_get_index(self, value, idx);
      if (val->type->kind == CUBEC_VALUE_TYPE_ERROR) {
        cubec_allocator_free(self->allocator, items);
        return val;
      }
      val = cubec_context_to_str(self, val);
      if (val->type->kind == CUBEC_VALUE_TYPE_ERROR) {
        cubec_allocator_free(self->allocator, items);
        return val;
      }
      const char **str = val->data;
      cubec_array_push(items, self->allocator, (void *)*str);
      len += strlen(*str);
    }
    char str[strlen(value->type->name) + len + 32];
    size_t offset = 0;
    strcpy(&str[offset], value->type->name);
    offset += strlen(value->type->name);
    str[offset++] = '[';
    for (size_t idx = 0; idx < cubec_array_get_size(items); idx++) {
      if (idx != 0) {
        strcpy(&str[offset], ", ");
        offset += 2;
      }
      const char *s = cubec_array_get_index(items, idx);
      strcpy(&str[offset], s);
      offset += strlen(s);
    }
    str[offset++] = ']';
    str[offset] = 0;
    cubec_allocator_free(self->allocator, items);
    return cubec_context_create_str(self, str, NULL);
  }
  case CUBEC_VALUE_TYPE_FUNCTION: {
    char str[strlen(value->type->name) + 32];
    sprintf(str, "%s(0x%" PRIxPTR ")", value->type->name,
            (intptr_t)value->data);
    return cubec_context_create_str(self, str, NULL);
  }
  case CUBEC_VALUE_TYPE_ENUM: {
    cubec_value_t val = cubec_context_resolve_enum(self, value, NULL);
    val = cubec_context_to_str(self, val);
    if (val->type->kind == CUBEC_VALUE_TYPE_ERROR) {
      return val;
    }
    const char **s = val->data;
    char str[strlen(*s) + strlen(value->type->name) + 32];
    sprintf(str, "%s{%s}", val->type->name, *s);
    return cubec_context_create_str(self, str, NULL);
  }
  case CUBEC_VALUE_TYPE_UNION: {
    cubec_union_data_t data = value->data;
    cubec_value_t value =
        cubec_context_create_value(self, data->type, &data->data, NULL);
    return cubec_context_to_str(self, value);
  };
  }
}

cubec_value_t cubec_context_get_length(cubec_context_t self,
                                       cubec_value_t value) {
  if (value->type->kind == CUBEC_VALUE_TYPE_ARRAY) {
    return cubec_context_create_uint64(
        self, ((cubec_array_meta_t)value->type)->length, NULL);
  } else if (value->type->kind == CUBEC_VALUE_TYPE_STRUCT) {
    cubec_value_t func = cubec_map_get(
        ((cubec_struct_meta_t)value->type->meta)->attributes, "__len__", NULL);
    if (func && func->type->kind == CUBEC_VALUE_TYPE_FUNCTION) {
      return cubec_context_call(self, func, 1, &value);
    }
  }
  char msg[strlen(value->type->name) + 32];
  sprintf(msg, "Cannot get length of %s", value->type->name);
  return cubec_context_create_error(self, msg, NULL);
}
cubec_value_t cubec_context_to_uint64(cubec_context_t self,
                                      cubec_value_t value) {
  if ((value->type->kind >= CUBEC_VALUE_TYPE_INT8 &&
       value->type->kind <= CUBEC_VALUE_TYPE_UINT64) ||
      value->type->kind == CUBEC_VALUE_TYPE_FLOAT32 ||
      value->type->kind == CUBEC_VALUE_TYPE_FLOAT64) {
    uint64_t val = 0;
    switch (value->type->kind) {
    case CUBEC_VALUE_TYPE_INT8:
      val = *(int8_t *)value->data;
      break;
    case CUBEC_VALUE_TYPE_INT16:
      val = *(int16_t *)value->data;
      break;
    case CUBEC_VALUE_TYPE_INT32:
      val = *(int32_t *)value->data;
      break;
    case CUBEC_VALUE_TYPE_INT64:
      val = *(int64_t *)value->data;
      break;
    case CUBEC_VALUE_TYPE_UINT8:
      val = *(uint8_t *)value->data;
      break;
    case CUBEC_VALUE_TYPE_UINT16:
      val = *(uint16_t *)value->data;
      break;
    case CUBEC_VALUE_TYPE_UINT32:
      val = *(uint32_t *)value->data;
      break;
    case CUBEC_VALUE_TYPE_UINT64:
      val = *(uint64_t *)value->data;
      break;
    case CUBEC_VALUE_TYPE_FLOAT32:
      val = *(float *)value->data;
      break;
    case CUBEC_VALUE_TYPE_FLOAT64:
      val = *(double *)value->data;
      break;
    default:
      break;
    }
    return cubec_context_create_uint64(self, val, NULL);
  } else {
    char msg[strlen(value->type->name) + 32];
    sprintf(msg, "Cannot convert '%s' to uint64", value->type->name);
    return cubec_context_create_error(self, msg, NULL);
  }
}
cubec_value_t cubec_context_to_boolean(cubec_context_t self,
                                       cubec_value_t value) {
  switch (value->type->kind) {
  case CUBEC_VALUE_TYPE_ERROR:
    return value;
  case CUBEC_VALUE_TYPE_UNDEFINED:
    return cubec_context_create_boolean(self, false, NULL);
  case CUBEC_VALUE_TYPE_INT8:
    return cubec_context_create_boolean(self, *(int8_t *)value->data, NULL);
  case CUBEC_VALUE_TYPE_INT16:
    return cubec_context_create_boolean(self, *(int16_t *)value->data, NULL);
  case CUBEC_VALUE_TYPE_INT32:
    return cubec_context_create_boolean(self, *(int32_t *)value->data, NULL);
  case CUBEC_VALUE_TYPE_INT64:
    return cubec_context_create_boolean(self, *(int64_t *)value->data, NULL);
  case CUBEC_VALUE_TYPE_UINT8:
    return cubec_context_create_boolean(self, *(uint8_t *)value->data, NULL);
  case CUBEC_VALUE_TYPE_UINT16:
    return cubec_context_create_boolean(self, *(uint16_t *)value->data, NULL);
  case CUBEC_VALUE_TYPE_UINT32:
    return cubec_context_create_boolean(self, *(uint32_t *)value->data, NULL);
  case CUBEC_VALUE_TYPE_UINT64:
    return cubec_context_create_boolean(self, *(uint64_t *)value->data, NULL);
  case CUBEC_VALUE_TYPE_FLOAT32:
    return cubec_context_create_boolean(self, *(float *)value->data, NULL);
  case CUBEC_VALUE_TYPE_FLOAT64:
    return cubec_context_create_boolean(self, *(double *)value->data, NULL);
  case CUBEC_VALUE_TYPE_BOOLEAN:
    return cubec_context_create_boolean(self, *(bool *)value->data, NULL);
  case CUBEC_VALUE_TYPE_STR:
    return cubec_context_create_boolean(self, true, NULL);
  case CUBEC_VALUE_TYPE_OPAQUE:
    return cubec_context_create_boolean(self, *(void **)value->data, NULL);
  case CUBEC_VALUE_TYPE_PTR:
    return cubec_context_create_boolean(self, *(void **)value->data, NULL);
  case CUBEC_VALUE_TYPE_PTR_ARRAY:
    return cubec_context_create_boolean(self, *(void **)value->data, NULL);
  case CUBEC_VALUE_TYPE_REF: {
    cubec_value_t *val = value->data;
    return cubec_context_to_boolean(self, *val);
  }
  default: {
    char msg[strlen(value->type->name) + 128];
    sprintf(msg, "Cannot convert '%s' to boolean", value->type->name);
    return cubec_context_create_error(self, msg, NULL);
  }
  }
}