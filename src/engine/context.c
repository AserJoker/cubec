#include "engine/context.h"
#include "ast/node_type.h"
#include "ast/program.h"
#include "c/program.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/compare.h"
#include "core/map.h"
#include "core/string.h"
#include "engine/array.h"
#include "engine/enum.h"
#include "engine/function.h"
#include "engine/module.h"
#include "engine/ptr.h"
#include "engine/result.h"
#include "engine/scope.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/union.h"
#include "engine/value.h"
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static void cubec_context_dispose(cubec_context_t self,
                                  cubec_allocator_t allocator) {
  while (self->current) {
    cubec_context_pop_scope(self);
  }
  cubec_allocator_free(allocator, self->modules);
  cubec_allocator_free(allocator, self->global);
  cubec_allocator_free(allocator, self->strings);
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

  self->eval_mode = CUBEC_EVAL_RUNTIME;

  self->root = cubec_create_scope(allocator, NULL);
  self->current = self->root;

  self->module = cubec_create_module(allocator, NULL);
  self->global = self->module;
  self->type_type = NULL;
  self->type_type = cubec_context_create_type(self, CUBEC_TYPE_KIND_TYPE,
                                              sizeof(void *), NULL, NULL);
  cubec_context_create_value(self, self->type_type, false, &self->type_type,
                             "type");

  self->type_error = cubec_context_create_type(
      self, CUBEC_TYPE_KIND_ERROR, sizeof(const char *), NULL, NULL);
  self->type_void =
      cubec_context_create_type(self, CUBEC_TYPE_KIND_VOID, 0, NULL, "void");
  self->value_undefined = cubec_context_create_value(self, self->type_void,
                                                     false, NULL, "undefined");
  self->type_int8 = cubec_context_create_type(self, CUBEC_TYPE_KIND_INT8,
                                              sizeof(int8_t), NULL, "int8");
  self->type_int16 = cubec_context_create_type(self, CUBEC_TYPE_KIND_INT16,
                                               sizeof(int16_t), NULL, "int16");
  self->type_int32 = cubec_context_create_type(self, CUBEC_TYPE_KIND_INT32,
                                               sizeof(int32_t), NULL, "int32");
  self->type_int64 = cubec_context_create_type(self, CUBEC_TYPE_KIND_INT64,
                                               sizeof(int64_t), NULL, "int64");
  self->type_uint8 = cubec_context_create_type(self, CUBEC_TYPE_KIND_UINT8,
                                               sizeof(uint8_t), NULL, "uint8");
  self->type_uint16 = cubec_context_create_type(
      self, CUBEC_TYPE_KIND_UINT16, sizeof(uint16_t), NULL, "uint16");
  self->type_uint32 = cubec_context_create_type(
      self, CUBEC_TYPE_KIND_UINT32, sizeof(uint32_t), NULL, "uint32");
  self->type_uint64 = cubec_context_create_type(
      self, CUBEC_TYPE_KIND_UINT64, sizeof(uint64_t), NULL, "uint64");
  self->type_float32 = cubec_context_create_type(
      self, CUBEC_TYPE_KIND_UINT32, sizeof(uint32_t), NULL, "float32");
  self->type_float64 = cubec_context_create_type(
      self, CUBEC_TYPE_KIND_UINT64, sizeof(uint64_t), NULL, "float64");
  self->type_boolean = cubec_context_create_type(self, CUBEC_TYPE_KIND_BOOLEAN,
                                                 sizeof(bool), NULL, "boolean");
  self->type_str = cubec_context_create_type(self, CUBEC_TYPE_KIND_STR,
                                             sizeof(const char *), NULL, "str");
  self->type_opaque = cubec_context_create_type(self, CUBEC_TYPE_KIND_OPAQUE,
                                                sizeof(void *), NULL, "opaque");
  return self;
}

cubec_type_t cubec_context_create_type(cubec_context_t self,
                                       cubec_type_kind_t kind, size_t size,
                                       void *meta, const char *name) {
  cubec_type_t type = cubec_create_type(self->allocator, kind, size, meta);
  cubec_array_push(self->module->types, type);
  if (self->type_type && name) {
    cubec_context_create_value(self, self->type_type, false, &type, name);
  }
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
  cubec_scope_t scope = self->current;
  self->current = self->current->parent;
  cubec_allocator_free(self->allocator, scope);
}
cubec_module_t cubec_context_get_module(cubec_context_t self,
                                        const char *name) {
  return cubec_map_get(self->modules, name, NULL);
}
bool cubec_context_has_module(cubec_context_t self, const char *name) {
  return cubec_map_has(self->modules, name, NULL);
}
cubec_value_t cubec_context_load_module(cubec_context_t self,
                                        const char *name) {
  FILE *fp = fopen(name, "rb");
  if (!fp) {
    return cubec_context_create_error(self, "Failed to open file: %s", name);
  }
  fseek(fp, 0, SEEK_END);
  size_t len = ftell(fp);
  fseek(fp, 0, SEEK_SET);
  char src[len + 1];
  fread(src, len, 1, fp);
  src[len] = 0;
  fclose(fp);
  cubec_position_t pos = {
      .column = 1,
      .line = 1,
      .offset = src,
  };
  cubec_ast_node_t node =
      cubec_read_ast_program(self->allocator, &pos, src + len);
  if (node->type == CUBEC_NODE_TYPE_ERROR) {
    cubec_ast_error_t error = (cubec_ast_error_t)node;
    cubec_value_t err = cubec_context_create_error(
        self, "Failed to compile %s: %s", name, error->message);
    cubec_allocator_free(self->allocator, node);
    return err;
  }
  cubec_module_t mod = cubec_create_module(self->allocator, name);
  cubec_map_set(self->modules, mod->filename, mod, NULL);
  mod->node = node;
  cubec_value_t err =
      cubec_c_write_program(self, (cubec_ast_program_t)node, name, &mod->data);
  return err;
}
cubec_module_t cubec_context_set_module(cubec_context_t self,
                                        cubec_module_t module) {
  cubec_module_t current = self->module;
  self->module = module;
  return current;
}
cubec_type_t cubec_context_create_ptr_type(cubec_context_t self,
                                           cubec_type_t type, bool is_mutable,
                                           bool is_volatile) {
  cubec_ptr_meta_t meta =
      cubec_create_ptr_meta(self->allocator, type, is_mutable, is_volatile);
  return cubec_context_create_type(self, CUBEC_TYPE_KIND_PTR, sizeof(void *),
                                   meta, NULL);
}
cubec_type_t cubec_context_create_ptr_array_type(cubec_context_t self,
                                                 cubec_type_t type,
                                                 bool is_mutable,
                                                 bool is_volatile) {
  cubec_ptr_meta_t meta =
      cubec_create_ptr_meta(self->allocator, type, is_mutable, is_volatile);
  return cubec_context_create_type(self, CUBEC_TYPE_KIND_PTR_ARRAY,
                                   sizeof(void *), meta, NULL);
}
cubec_type_t cubec_context_create_array_type(cubec_context_t self,
                                             cubec_type_t type, size_t length) {
  cubec_array_meta_t meta =
      cubec_create_array_meta(self->allocator, type, length);
  return cubec_context_create_type(self, CUBEC_TYPE_KIND_ARRAY,
                                   type->size * length, meta, NULL);
}
cubec_type_t cubec_context_create_struct_type(cubec_context_t self,
                                              size_t align, const char *name) {
  cubec_struct_meta_t meta =
      cubec_create_struct_meta(self->allocator, align, name);
  return cubec_context_create_type(self, CUBEC_TYPE_KIND_STRUCT, 1, meta, name);
}
cubec_type_t cubec_context_create_union_type(cubec_context_t self, size_t align,
                                             const char *name) {
  cubec_union_meta_t meta =
      cubec_create_union_meta(self->allocator, align, name);
  return cubec_context_create_type(self, CUBEC_TYPE_KIND_UNION, 1, meta, name);
}
cubec_value_t cubec_context_add_struct_field(cubec_context_t self,
                                             cubec_type_t stru,
                                             const char *name,
                                             cubec_type_t type) {
  if (stru->kind != CUBEC_TYPE_KIND_STRUCT) {
    char *type_name = cubec_context_type_to_string(self, stru);
    cubec_value_t error = cubec_context_create_error(
        self, "Cannot add field %s to %s", name, type_name);
    cubec_allocator_free(self->allocator, type_name);
    return error;
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
    char *type_name = cubec_context_type_to_string(self, stru);
    cubec_value_t error = cubec_context_create_error(
        self, "Cannot add field %s to %s", name, type_name);
    cubec_allocator_free(self->allocator, type_name);
    return error;
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
      (type->size > etype->size ? type->size : etype->size) + 1, meta, NULL);
  return result_type;
}
cubec_type_t cubec_context_create_function_type(cubec_context_t self,
                                                cubec_type_t type,
                                                size_t num_args,
                                                cubec_type_t *args,
                                                bool is_variadic) {
  cubec_function_meta_t meta = cubec_create_function_meta(
      self->allocator, type, num_args, args, is_variadic);
  return cubec_context_create_type(self, CUBEC_TYPE_KIND_FUNCTION,
                                   sizeof(void *), meta, NULL);
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
cubec_value_t cubec_context_create_float32(cubec_context_t self, float value,
                                           bool is_mutable, const char *name) {
  return cubec_context_create_value(self, self->type_uint64, is_mutable, &value,
                                    name);
}
cubec_value_t cubec_context_create_float64(cubec_context_t self, double value,
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
  cubec_type_t ptr_type =
      cubec_context_create_ptr_array_type(self, type, true, false);
  return cubec_context_create_value(self, ptr_type, is_mutable, &data, name);
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
cubec_value_t cubec_context_create_union(cubec_context_t self,
                                         cubec_type_t type, bool is_mutable,
                                         const char *name) {
  void *data = cubec_allocator_alloc(self->allocator, type->size, NULL);
  memset(data, 0, type->size);
  return cubec_context_create_value(self, type, is_mutable, data, name);
}

bool cubec_context_check_type(cubec_context_t self, cubec_type_t dst,
                              cubec_type_t src) {
  if (src->kind == CUBEC_TYPE_KIND_ARRAY &&
      dst->kind == CUBEC_TYPE_KIND_PTR_ARRAY) {
    cubec_ptr_meta_t dst_meta = dst->meta;
    cubec_array_meta_t src_meta = src->meta;
    return cubec_context_check_type(self, dst_meta->type, src_meta->type);
  }
  if (dst->kind >= CUBEC_TYPE_KIND_INT8 &&
      dst->kind <= CUBEC_TYPE_KIND_UINT64 &&
      src->kind >= CUBEC_TYPE_KIND_INT8 &&
      src->kind <= CUBEC_TYPE_KIND_UINT64) {
    return true;
  }
  if (src->kind != dst->kind) {
    return false;
  }
  if (src->kind == CUBEC_TYPE_KIND_PTR ||
      src->kind == CUBEC_TYPE_KIND_PTR_ARRAY) {
    cubec_ptr_meta_t dst_meta = dst->meta;
    cubec_ptr_meta_t src_meta = src->meta;
    if (dst_meta->is_mutable && !src_meta->is_mutable) {
      return false;
    }
    return cubec_context_check_type(self, dst_meta->type, src_meta->type);
  }
  if (src->kind == CUBEC_TYPE_KIND_ARRAY) {
    cubec_array_meta_t dst_meta = dst->meta;
    cubec_array_meta_t src_meta = src->meta;
    return cubec_context_check_type(self, dst_meta->type, src_meta->type);
  }
  if (src->kind == CUBEC_TYPE_KIND_ENUM) {
    cubec_enum_meta_t dst_meta = dst->meta;
    cubec_enum_meta_t src_meta = src->meta;
    if (cubec_array_get_size(dst_meta->options) !=
        cubec_array_get_size(src_meta->options)) {
      return false;
    }
    if (!cubec_context_check_type(self, dst_meta->type, src_meta->type)) {
      return false;
    }
    for (size_t idx = 0; idx < cubec_array_get_size(dst_meta->options); idx++) {
      cubec_enum_option_t dst_option =
          cubec_array_get_index(dst_meta->options, idx);
      cubec_enum_option_t src_option =
          cubec_array_get_index(src_meta->options, idx);
      if (strcmp(dst_option->name, src_option->name) != 0) {
        return false;
      }
      if (memcmp(dst_option->value, src_option->value, dst_meta->type->size) !=
          0) {
        return false;
      }
    }
  }
  if (src->kind == CUBEC_TYPE_KIND_STRUCT) {
    cubec_struct_meta_t dst_meta = dst->meta;
    cubec_struct_meta_t src_meta = src->meta;
    if (dst_meta->align != src_meta->align) {
      return false;
    }
    if (cubec_array_get_size(dst_meta->fields) !=
        cubec_array_get_size(src_meta->fields)) {
      return false;
    }
    for (size_t idx = 0; idx < cubec_array_get_size(dst_meta->fields); idx++) {
      cubec_struct_field_t dst_field =
          cubec_array_get_index(dst_meta->fields, idx);
      cubec_struct_field_t src_field =
          cubec_array_get_index(src_meta->fields, idx);
      if (strcmp(dst_field->name, src_field->name) != 0) {
        return false;
      }
      if (dst_field->offset != src_field->offset) {
        return false;
      }
      if (!cubec_context_check_type(self, dst_field->type, src_field->type)) {
        return false;
      }
    }
  }
  if (src->kind == CUBEC_TYPE_KIND_FUNCTION) {
    cubec_function_meta_t dst_meta = dst->meta;
    cubec_function_meta_t src_meta = src->meta;
    if (!cubec_context_check_type(self, dst_meta->type, src_meta->type)) {
      return false;
    }
    if (cubec_array_get_size(dst_meta->args) !=
        cubec_array_get_size(src_meta->args)) {
      return false;
    }
    if (dst_meta->is_variadic != src_meta->is_variadic) {
      return false;
    }
    for (size_t idx = 0; idx < cubec_array_get_size(dst_meta->args); idx++) {
      cubec_type_t dst_arg = cubec_array_get_index(dst_meta->args, idx);
      cubec_type_t src_arg = cubec_array_get_index(src_meta->args, idx);
      if (!cubec_context_check_type(self, dst_arg, src_arg)) {
        return false;
      }
    }
  }
  if (src->kind == CUBEC_TYPE_KIND_UNION) {
    cubec_union_meta_t dst_meta = dst->meta;
    cubec_union_meta_t src_meta = dst->meta;
    if (dst_meta->align != src_meta->align) {
      return false;
    }
    if (cubec_array_get_size(dst_meta->fields) !=
        cubec_array_get_size(src_meta->fields)) {
      return false;
    }
    for (size_t idx = 0; idx < cubec_array_get_size(dst_meta->fields); idx++) {
      cubec_union_field_t dst_field =
          cubec_array_get_index(dst_meta->fields, idx);
      cubec_union_field_t src_field = NULL;
      for (size_t iidx = 0; iidx < cubec_array_get_size(src_meta->fields);
           iidx++) {
        src_field = cubec_array_get_index(src_meta->fields, iidx);
        if (strcmp(dst_field->name, src_field->name) != 0) {
          break;
        }
      }
      if (!src_field) {
        return false;
      }
      if (!cubec_context_check_type(self, dst_field->type, src_field->type)) {
        return false;
      }
    }
  }
  return true;
}
char *cubec_context_type_to_string(cubec_context_t self, cubec_type_t type) {
  switch (type->kind) {
  case CUBEC_TYPE_KIND_TYPE:
    return cubec_create_cstring(self->allocator, "type");
  case CUBEC_TYPE_KIND_ERROR:
    return cubec_create_cstring(self->allocator, "error");
  case CUBEC_TYPE_KIND_VOID:
    return cubec_create_cstring(self->allocator, "void");
  case CUBEC_TYPE_KIND_INT8:
    return cubec_create_cstring(self->allocator, "int8");
  case CUBEC_TYPE_KIND_INT16:
    return cubec_create_cstring(self->allocator, "int16");
  case CUBEC_TYPE_KIND_INT32:
    return cubec_create_cstring(self->allocator, "int32");
  case CUBEC_TYPE_KIND_INT64:
    return cubec_create_cstring(self->allocator, "int64");
  case CUBEC_TYPE_KIND_UINT8:
    return cubec_create_cstring(self->allocator, "uint8");
  case CUBEC_TYPE_KIND_UINT16:
    return cubec_create_cstring(self->allocator, "uint16");
  case CUBEC_TYPE_KIND_UINT32:
    return cubec_create_cstring(self->allocator, "uint32");
  case CUBEC_TYPE_KIND_UINT64:
    return cubec_create_cstring(self->allocator, "uint64");
  case CUBEC_TYPE_KIND_FLOAT32:
    return cubec_create_cstring(self->allocator, "float32");
  case CUBEC_TYPE_KIND_FLOAT64:
    return cubec_create_cstring(self->allocator, "float64");
  case CUBEC_TYPE_KIND_BOOLEAN:
    return cubec_create_cstring(self->allocator, "boolean");
  case CUBEC_TYPE_KIND_STR:
    return cubec_create_cstring(self->allocator, "str");
  case CUBEC_TYPE_KIND_OPAQUE:
    return cubec_create_cstring(self->allocator, "opaque");
  case CUBEC_TYPE_KIND_PTR: {
    cubec_ptr_meta_t meta = type->meta;
    char *body = cubec_context_type_to_string(self, meta->type);
    char *s = cubec_allocator_alloc(self->allocator, strlen(body) + 32, NULL);
    sprintf(s, "*%s%s %s", !meta->is_mutable ? " const" : "",
            meta->is_volatile ? " volatile" : "", body);
    cubec_allocator_free(self->allocator, body);
    return s;
  }
  case CUBEC_TYPE_KIND_PTR_ARRAY: {
    cubec_ptr_meta_t meta = type->meta;
    char *body = cubec_context_type_to_string(self, meta->type);
    char *s = cubec_allocator_alloc(self->allocator, strlen(body) + 32, NULL);
    sprintf(s, "[*]%s%s %s", !meta->is_mutable ? " const" : "",
            meta->is_volatile ? " volatile" : "", body);
    cubec_allocator_free(self->allocator, body);
    return s;
  }
  case CUBEC_TYPE_KIND_ARRAY: {
    cubec_array_meta_t meta = type->meta;
    char *body = cubec_context_type_to_string(self, meta->type);
    char *s = cubec_allocator_alloc(self->allocator, strlen(body) + 32, NULL);
    sprintf(s, "[%" PRIuPTR "]%s", meta->len, body);
    cubec_allocator_free(self->allocator, body);
    return s;
  }
  case CUBEC_TYPE_KIND_STRUCT: {
    cubec_struct_meta_t meta = type->meta;
    size_t len = 0;
    if (meta->name) {
      len += strlen(meta->name);
    }
    len += 32;
    char *s = cubec_allocator_alloc(self->allocator, len, NULL);
    sprintf(s, "struct %s{...} align(%" PRIuPTR ")",
            meta->name ? meta->name : "", meta->align);
    return s;
  }
  case CUBEC_TYPE_KIND_UNION: {
    cubec_union_meta_t meta = type->meta;
    size_t len = 0;
    if (meta->name) {
      len += strlen(meta->name);
    }
    len += 32;
    char *s = cubec_allocator_alloc(self->allocator, len, NULL);
    sprintf(s, "union %s{...} align(%" PRIuPTR ")",
            meta->name ? meta->name : "", meta->align);
    return s;
  }
  case CUBEC_TYPE_KIND_ENUM: {
    cubec_enum_meta_t meta = type->meta;
    char *base = cubec_context_type_to_string(self, meta->type);
    size_t len = strlen(base);
    if (meta->name) {
      len += strlen(meta->name);
    }
    char *s = cubec_allocator_alloc(self->allocator, len + 32, NULL);
    sprintf(s, "enum %s:%s {...}", meta->name ? meta->name : "", base);
    cubec_allocator_free(self->allocator, base);
    return s;
  }
  case CUBEC_TYPE_KIND_RESULT: {
    cubec_result_meta_t meta = type->meta;
    char *err_name = cubec_context_type_to_string(self, meta->error_type);
    char *val_name = cubec_context_type_to_string(self, meta->type);
    size_t len = strlen(err_name) + strlen(val_name) + 32;
    char *s = cubec_allocator_alloc(self->allocator, len, NULL);
    sprintf(s, "result(%s,%s)", val_name, err_name);
    cubec_allocator_free(self->allocator, err_name);
    cubec_allocator_free(self->allocator, val_name);
    return s;
  }
  case CUBEC_TYPE_KIND_FUNCTION: {
    cubec_function_meta_t meta = type->meta;
    size_t len = 0;
    char *r_name = cubec_context_type_to_string(self, meta->type);
    len += strlen(r_name);
    cubec_array_initialize_t initialize = {
        .autofree = true,
    };
    cubec_array_t arr = cubec_create_array(self->allocator, &initialize);
    for (size_t idx = 0; cubec_array_get_size(meta->args); idx++) {
      cubec_type_t arg = cubec_array_get_index(meta->args, idx);
      char *s = cubec_context_type_to_string(self, arg);
      len += strlen(s);
      len++;
      cubec_array_push(arr, s);
    }
    len += 32;
    char *s = cubec_allocator_alloc(self->allocator, len, NULL);
    size_t offset = 0;
    strcpy(&s[offset], "func");
    offset += 4;
    s[offset++] = '(';
    for (size_t idx = 0; idx < cubec_array_get_size(arr); idx++) {
      if (idx != 0) {
        s[offset++] = ',';
        s[offset++] = ' ';
      }
      char *arg = cubec_array_get_index(arr, idx);
      strcpy(&s[offset], arg);
      offset += strlen(arg);
    }
    if (meta->is_variadic) {
      strcpy(&s[offset], "...");
      offset += 3;
    } else if (!cubec_array_get_size(arr)) {
      strcpy(&s[offset], "void");
    }
    s[offset++] = ')';
    s[offset++] = ':';
    s[offset++] = ' ';
    strcpy(&s[offset], r_name);
    offset += strlen(r_name);
    s[offset] = 0;
    cubec_allocator_free(self->allocator, arr);
    cubec_allocator_free(self->allocator, r_name);
    return s;
  }
  case CUBEC_TYPE_KIND_TEMPLATE:
    break;
  }
  return NULL;
}

cubec_value_t cubec_context_create_error(cubec_context_t self, const char *fmt,
                                         ...) {
  va_list args;
  va_start(args, fmt);
  size_t len = vsnprintf(NULL, 0, fmt, args);
  char *msg = cubec_allocator_alloc(self->allocator, len + 1, NULL);
  va_end(args);
  va_start(args, fmt);
  vsprintf(msg, fmt, args);
  va_end(args);
  cubec_array_push(self->strings, msg);
  return cubec_context_create_value(self, self->type_error, false, &msg, NULL);
}
cubec_value_t cubec_context_create_compile_error(cubec_context_t self,
                                                 cubec_ast_node_t node,
                                                 const char *filename,
                                                 const char *fmt, ...) {
  char num[16];
  sprintf(num, "%" PRIuPTR " | ", node->loc.begin.line);
  char *line = cubec_location_get_line(node->loc, self->allocator);
  size_t len = strlen(line);
  size_t column = node->loc.begin.column - 1;
  len += strlen(num);
  char marks[len + 1];
  memset(marks, 0, len + 1);
  for (size_t id = 0; id < len; id++) {
    if (id < column + strlen(num)) {
      marks[id] = ' ';
    } else if (id < node->loc.end.column - 1 + strlen(num)) {
      marks[id] = '^';
    }
  }
  va_list args;
  va_start(args, fmt);
  len = vsnprintf(NULL, 0, fmt, args);
  char *msg = cubec_allocator_alloc(self->allocator, len + 1, NULL);
  va_end(args);
  va_start(args, fmt);
  vsprintf(msg, fmt, args);
  va_end(args);
  cubec_value_t err =
      cubec_context_create_error(self,
                                 "%s:%" PRIuPTR ":%" PRIuPTR ": error: %s\n"
                                 "%s%s\n"
                                 "%s\n",
                                 filename, node->loc.begin.line,
                                 node->loc.begin.column, msg, num, line, marks);
  cubec_allocator_free(self->allocator, line);
  cubec_allocator_free(self->allocator, msg);
  return err;
}

cubec_value_t cubec_context_convert_compile_error(cubec_context_t self,
                                                  cubec_ast_node_t node,
                                                  const char *filename,
                                                  cubec_value_t error) {
  if (error->type->kind != CUBEC_TYPE_KIND_ERROR) {
    return cubec_context_create_compile_error(self, node, filename,
                                              "Invalid compile error");
  }
  return cubec_context_create_compile_error(self, node, filename,
                                            *(const char **)error->data);
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
cubec_value_t cubec_context_create_function(cubec_context_t self,
                                            cubec_type_t type,
                                            cubec_ast_node_t node,
                                            bool is_mutable, const char *name) {
  cubec_function_desc_t func = cubec_create_function_desc(
      self->allocator, CUBEC_FUNCTION_RUNTIME, node, name);
  cubec_array_push(self->module->functions, func);
  return cubec_context_create_value(self, type, is_mutable, &func, name);
}
cubec_value_t cubec_context_create_native(cubec_context_t self,
                                          cubec_type_t type,
                                          cubec_native_handle_t native,
                                          bool is_mutable, const char *name) {
  cubec_function_desc_t func = cubec_create_function_desc(
      self->allocator, CUBEC_FUNCTION_NATIVE, native, name);
  cubec_array_push(self->module->functions, func);
  return cubec_context_create_value(self, type, is_mutable, &func, name);
}
cubec_value_t cubec_context_create_type_value(cubec_context_t self,
                                              cubec_type_t type,
                                              bool is_mutable,
                                              const char *name) {
  return cubec_context_create_value(self, self->type_type, is_mutable, &type,
                                    name);
}
cubec_value_t cubec_context_set_index(cubec_context_t self, cubec_value_t arr,
                                      size_t idx, cubec_value_t value) {
  if (!arr->is_mutable) {
    return cubec_context_create_error(self,
                                      "Read-only variable is not assignable");
  }
  cubec_type_t dst_type = arr->type;
  void *dst = arr->data;
  cubec_type_t src_type = value->type;
  void *src = value->data;
  if (dst_type->kind == CUBEC_TYPE_KIND_ARRAY) {
    cubec_array_meta_t meta = dst_type->meta;
    if (meta->type->kind == CUBEC_TYPE_KIND_PTR_ARRAY &&
        value->type->kind == CUBEC_TYPE_KIND_ARRAY) {
      if (value->data) {
        value = cubec_context_create_value(self, meta->type, value->is_mutable,
                                           &value->data, NULL);
      } else {
        value = cubec_context_create_value(self, meta->type, value->is_mutable,
                                           NULL, NULL);
      }
    }
    if (!cubec_context_check_type(self, meta->type, src_type)) {
      char *dst_name = cubec_context_type_to_string(self, meta->type);
      char *src_name = cubec_context_type_to_string(self, value->type);
      cubec_value_t error = cubec_context_create_error(
          self, "Cannot convert %s to %s", src_name, dst_name);
      cubec_allocator_free(self->allocator, dst_name);
      cubec_allocator_free(self->allocator, src_name);
      return error;
    }
    if (idx >= meta->len) {
      return cubec_context_create_error(self, "Out of range");
    }
    if (!dst || !src) {
      return cubec_context_create_value(self, value->type, value->is_mutable,
                                        NULL, NULL);
    }
    uint8_t *offset = dst + meta->type->size * idx;
    memcpy(offset, src, src_type->size);
    return cubec_context_create_value(self, value->type, value->is_mutable,
                                      value->data, NULL);
  }
  if (dst_type->kind == CUBEC_TYPE_KIND_PTR_ARRAY) {
    cubec_ptr_meta_t meta = dst_type->meta;
    if (meta->type->kind == CUBEC_TYPE_KIND_PTR_ARRAY &&
        value->type->kind == CUBEC_TYPE_KIND_ARRAY) {
      if (value->data) {
        value = cubec_context_create_value(self, meta->type, value->is_mutable,
                                           &value->data, NULL);
      } else {
        value = cubec_context_create_value(self, meta->type, value->is_mutable,
                                           NULL, NULL);
      }
    }
    if (!cubec_context_check_type(self, meta->type, src_type)) {
      char *dst_name = cubec_context_type_to_string(self, meta->type);
      char *src_name = cubec_context_type_to_string(self, value->type);
      cubec_value_t error = cubec_context_create_error(
          self, "Cannot convert %s to %s", src_name, dst_name);
      cubec_allocator_free(self->allocator, dst_name);
      cubec_allocator_free(self->allocator, src_name);
      return error;
    }
    if (!dst || !src) {
      return cubec_context_create_value(self, value->type, value->is_mutable,
                                        NULL, NULL);
    }
    uint8_t *offset = dst + meta->type->size * idx;
    memcpy(offset, src, src_type->size);
    return self->value_undefined;
  }
  return cubec_context_create_error(
      self, "Subscripted value is not an array, ptr array");
}
cubec_value_t cubec_context_get_index(cubec_context_t self, cubec_value_t arr,
                                      size_t idx) {
  cubec_type_t type = arr->type;
  void *dst = arr->data;
  if (type->kind == CUBEC_TYPE_KIND_ARRAY) {
    cubec_array_meta_t meta = type->meta;
    if (meta->len <= idx) {
      return cubec_context_create_error(self, "Out of range");
    }
    if (!dst) {
      return cubec_context_create_value(self, meta->type, arr->is_mutable, NULL,
                                        NULL);
    }
    uint8_t *start = dst + idx * meta->type->size;
    return cubec_context_create_value(self, meta->type, arr->is_mutable, start,
                                      NULL);
  }
  if (type->kind == CUBEC_TYPE_KIND_PTR_ARRAY) {
    cubec_ptr_meta_t meta = type->meta;
    if (!dst) {
      return cubec_context_create_value(self, meta->type, arr->is_mutable, NULL,
                                        NULL);
    }
    uint8_t *start = *(uint8_t **)dst + idx * meta->type->size;
    return cubec_context_create_value(self, meta->type, arr->is_mutable, start,
                                      NULL);
  }
  return cubec_context_create_error(
      self, "Subscripted value is not an array, ptr array");
}
cubec_value_t cubec_context_set_field(cubec_context_t self, cubec_value_t obj,
                                      const char *name, cubec_value_t value) {
  if (!value->is_mutable) {
    return cubec_context_create_error(self,
                                      "Read-only variable is not assignable");
  }
  cubec_type_t type = obj->type;
  void *dst = obj->data;
  if (type->kind == CUBEC_TYPE_KIND_PTR) {
    cubec_ptr_meta_t meta = type->meta;
    type = meta->type;
    if (dst) {
      dst = *(void **)dst;
    }
  }
  if (type->kind == CUBEC_TYPE_KIND_STRUCT) {
    cubec_struct_meta_t meta = type->meta;
    for (size_t idx = 0; idx < cubec_array_get_size(meta->fields); idx++) {
      cubec_struct_field_t field = cubec_array_get_index(meta->fields, idx);
      if (strcmp(field->name, name) == 0) {
        if (field->type->kind == CUBEC_TYPE_KIND_PTR_ARRAY &&
            value->type->kind == CUBEC_TYPE_KIND_ARRAY) {
          if (value->data) {
            value = cubec_context_create_value(
                self, field->type, value->is_mutable, &value->data, NULL);
          } else {
            value = cubec_context_create_value(self, field->type,
                                               value->is_mutable, NULL, NULL);
          }
        }
        if (!cubec_context_check_type(self, field->type, value->type)) {
          char *dst_name = cubec_context_type_to_string(self, field->type);
          char *src_name = cubec_context_type_to_string(self, value->type);
          cubec_value_t error = cubec_context_create_error(
              self, "Cannot convert %s to %s", src_name, dst_name);
          cubec_allocator_free(self->allocator, dst_name);
          cubec_allocator_free(self->allocator, src_name);
          return error;
        }
        if (!dst || !value->data) {
          return cubec_context_create_value(self, value->type,
                                            value->is_mutable, NULL, NULL);
        } else {
          memcpy(dst + field->offset, value->data, value->type->size);
          return cubec_context_create_value(
              self, value->type, value->is_mutable, value->data, NULL);
        }
      }
    }
  } else if (type->kind == CUBEC_TYPE_KIND_UNION) {
    cubec_union_meta_t meta = type->meta;
    for (size_t idx = 0; idx < cubec_array_get_size(meta->fields); idx++) {
      cubec_union_field_t field = cubec_array_get_index(meta->fields, idx);
      if (strcmp(field->name, name) == 0) {
        if (field->type->kind == CUBEC_TYPE_KIND_PTR_ARRAY &&
            value->type->kind == CUBEC_TYPE_KIND_ARRAY) {
          if (value->data) {
            value = cubec_context_create_value(
                self, field->type, value->is_mutable, &value->data, NULL);
          } else {
            value = cubec_context_create_value(self, field->type,
                                               value->is_mutable, NULL, NULL);
          }
        }
        if (!cubec_context_check_type(self, field->type, value->type)) {
          char *dst_name = cubec_context_type_to_string(self, field->type);
          char *src_name = cubec_context_type_to_string(self, value->type);
          cubec_value_t error = cubec_context_create_error(
              self, "Cannot convert %s to %s", src_name, dst_name);
          cubec_allocator_free(self->allocator, dst_name);
          cubec_allocator_free(self->allocator, src_name);
          return error;
        }
        if (!dst || !value->data) {
          return cubec_context_create_value(self, value->type,
                                            value->is_mutable, NULL, NULL);
        } else {
          memcpy(dst, value->data, value->type->size);
          return cubec_context_create_value(
              self, value->type, value->is_mutable, value->data, NULL);
        }
      }
    }
  }
  char *type_name = cubec_context_type_to_string(self, obj->type);
  cubec_value_t error = cubec_context_create_error(
      self, "No member named '%s' in '%s'", name, type_name);
  cubec_allocator_free(self->allocator, type_name);
  return error;
}

cubec_value_t cubec_context_get_field(cubec_context_t self, cubec_value_t obj,
                                      const char *name) {
  cubec_type_t type = obj->type;
  void *dst = obj->data;
  if (type->kind == CUBEC_TYPE_KIND_PTR) {
    cubec_ptr_meta_t meta = type->meta;
    type = meta->type;
    if (dst) {
      dst = *(void **)dst;
    }
  }
  if (type->kind == CUBEC_TYPE_KIND_STRUCT) {
    cubec_struct_meta_t meta = type->meta;
    for (size_t idx = 0; idx < cubec_array_get_size(meta->fields); idx++) {
      cubec_struct_field_t field = cubec_array_get_index(meta->fields, idx);
      if (strcmp(field->name, name) == 0) {
        if (!dst) {
          return cubec_context_create_value(self, field->type, obj->is_mutable,
                                            NULL, NULL);
        }
        return cubec_context_create_value(self, field->type, obj->is_mutable,
                                          dst + field->offset, NULL);
      }
    }
  }
  if (type->kind == CUBEC_TYPE_KIND_UNION) {
    cubec_union_meta_t meta = type->meta;
    for (size_t idx = 0; idx < cubec_array_get_size(meta->fields); idx++) {
      cubec_union_field_t field = cubec_array_get_index(meta->fields, idx);
      if (strcmp(field->name, name) == 0) {
        if (!dst) {
          return cubec_context_create_value(self, field->type, obj->is_mutable,
                                            NULL, NULL);
        }
        return cubec_context_create_value(self, field->type, obj->is_mutable,
                                          dst, NULL);
      }
    }
  }
  char *type_name = cubec_context_type_to_string(self, obj->type);
  cubec_value_t error = cubec_context_create_error(
      self, "No member named '%s' in '%s'", name, type_name);
  cubec_allocator_free(self->allocator, type_name);
  return error;
}
cubec_value_t cubec_context_call(cubec_context_t self, cubec_value_t func,
                                 size_t argc, cubec_value_t *argv) {
  if (func->type->kind == CUBEC_TYPE_KIND_FUNCTION) {
    cubec_function_meta_t meta = func->type->meta;
    if (argc < cubec_array_get_size(meta->args)) {
      char *type_name = cubec_context_type_to_string(self, func->type);
      cubec_value_t error = cubec_context_create_error(
          self, "Cannot call value(type = %s) with %" PRIuPTR " arguments",
          type_name, argc);
      cubec_allocator_free(self->allocator, type_name);
      return error;
    }
    cubec_array_t arguments = cubec_create_array(self->allocator, NULL);
    cubec_context_push_scope(self);
    cubec_value_t result = NULL;
    for (size_t idx = 0; idx < argc; idx++) {
      cubec_value_t arg = argv[idx];
      if (idx >= cubec_array_get_size(meta->args)) {
        if (!meta->is_variadic) {
          char *type_name = cubec_context_type_to_string(self, func->type);
          result = cubec_context_create_error(
              self, "Cannot call value(type = %s) with %" PRIuPTR " arguments",
              type_name, argc);
          cubec_allocator_free(self->allocator, type_name);
          break;
        }
        cubec_array_push(
            arguments,
            cubec_context_create_value(self, arg->type, true, arg->data, NULL));
      } else {
        cubec_type_t dst = cubec_array_get_index(meta->args, idx);
        if (dst->kind == CUBEC_TYPE_KIND_PTR_ARRAY &&
            arg->type->kind == CUBEC_TYPE_KIND_ARRAY) {
          if (arg->data) {
            arg = cubec_context_create_value(self, dst, true, &arg->data, NULL);
          } else {
            arg = cubec_context_create_value(self, dst, true, NULL, NULL);
          }
        }
        if (!cubec_context_check_type(self, dst, argv[idx]->type)) {
          char *dst_name = cubec_context_type_to_string(self, dst);
          char *src_name = cubec_context_type_to_string(self, argv[idx]->type);
          result = cubec_context_create_error(
              self, "Cannot convert argument[%" PRIuPTR "] (type = %s) to %s",
              idx, src_name, dst_name);
          cubec_allocator_free(self->allocator, dst_name);
          cubec_allocator_free(self->allocator, src_name);
          break;
        }
        cubec_array_push(arguments, cubec_context_create_value(
                                        self, dst, true, arg->data, NULL));
      }
    }
    if (!result) {
      cubec_function_desc_t desc = *(cubec_function_desc_t *)func->data;
      if (desc->kind == CUBEC_FUNCTION_NATIVE) {
        result = desc->native(self, cubec_array_get_size(arguments),
                              cubec_array_get_data(arguments));
        if (!cubec_context_check_type(self, result->type, meta->type)) {
          char *dst_name = cubec_context_type_to_string(self, meta->type);
          char *src_name = cubec_context_type_to_string(self, result->type);
          result = cubec_context_create_error(
              self, "Cannot convert native function return type %s to %s",
              dst_name, src_name);
          cubec_allocator_free(self->allocator, dst_name);
          cubec_allocator_free(self->allocator, src_name);
        }
      } else if (desc->kind == CUBEC_FUNCTION_RUNTIME) {
        result = cubec_context_create_value(self, meta->type, true, NULL, NULL);
      } else if (desc->kind == CUBEC_FUNCTION_COMPTIME) {
        // TODO: eval desc->node;
      }
    }
    cubec_allocator_free(self->allocator, arguments);
    cubec_scope_t scope = self->current;
    self->current = self->current->parent;
    result = cubec_context_create_value(self, result->type, result->is_mutable,
                                        result->data, NULL);
    self->current = scope;
    cubec_context_pop_scope(self);
    return result;
  }
  char *type_name = cubec_context_type_to_string(self, func->type);
  cubec_value_t error = cubec_context_create_error(
      self, "Cannot call value(type = %s)", type_name);
  cubec_allocator_free(self->allocator, type_name);
  return error;
}
cubec_value_t cubec_context_load_value(cubec_context_t self, const char *name) {
  cubec_scope_t scope = self->current;
  while (scope) {
    cubec_value_t val = cubec_map_get(scope->variables, name, NULL);
    if (val) {
      return val;
    }
    scope = scope->parent;
  }
  return NULL;
}
cubec_value_t cubec_context_inc_value(cubec_context_t self,
                                      cubec_value_t value) {
  if (!value->is_mutable) {
    return cubec_context_create_error(self, "Expression is not assignable");
  }
  if (value->type->kind < CUBEC_TYPE_KIND_INT8 &&
      value->type->kind > CUBEC_TYPE_KIND_UINT64) {
    char *type_name = cubec_context_type_to_string(self, value->type);
    cubec_value_t err = cubec_context_create_error(
        self, "Cannot increment value of type '%s'", type_name);
    cubec_allocator_free(self->allocator, type_name);
    return err;
  }
  switch (value->type->kind) {
  case CUBEC_TYPE_KIND_INT8:
    (*(int8_t *)(value->data))++;
    break;
  case CUBEC_TYPE_KIND_INT16:
    (*(int16_t *)(value->data))++;
    break;
  case CUBEC_TYPE_KIND_INT32:
    (*(int32_t *)(value->data))++;
    break;
  case CUBEC_TYPE_KIND_INT64:
    (*(int64_t *)(value->data))++;
    break;
  case CUBEC_TYPE_KIND_UINT8:
    (*(uint8_t *)(value->data))++;
    break;
  case CUBEC_TYPE_KIND_UINT16:
    (*(uint16_t *)(value->data))++;
    break;
  case CUBEC_TYPE_KIND_UINT32:
    (*(uint32_t *)(value->data))++;
    break;
  case CUBEC_TYPE_KIND_UINT64:
    (*(uint64_t *)(value->data))++;
    break;
  default: {
    char *type_name = cubec_context_type_to_string(self, value->type);
    cubec_value_t err = cubec_context_create_error(
        self, "Cannot increment value of type '%s'", type_name);
    cubec_allocator_free(self->allocator, type_name);
    return err;
  }
  }
  return self->value_undefined;
}
int64_t cubec_context_value_to_int64(cubec_context_t self,
                                     cubec_value_t value) {
  switch (value->type->kind) {
  case CUBEC_TYPE_KIND_INT8:
    return *(int8_t *)value->data;
  case CUBEC_TYPE_KIND_INT16:
    return *(int16_t *)value->data;
  case CUBEC_TYPE_KIND_INT32:
    return *(int32_t *)value->data;
  case CUBEC_TYPE_KIND_INT64:
    return *(int64_t *)value->data;
  default:
    break;
  }
  return 0;
}
uint64_t cubec_context_value_to_uint64(cubec_context_t self,
                                       cubec_value_t value) {
  switch (value->type->kind) {
  case CUBEC_TYPE_KIND_UINT8:
    return *(uint8_t *)value->data;
  case CUBEC_TYPE_KIND_UINT16:
    return *(uint16_t *)value->data;
  case CUBEC_TYPE_KIND_UINT32:
    return *(uint32_t *)value->data;
  case CUBEC_TYPE_KIND_UINT64:
    return *(uint64_t *)value->data;
  default:
    break;
  }
  return 0;
}
cubec_value_t cubec_context_dec_value(cubec_context_t self,
                                      cubec_value_t value) {
  if (!value->is_mutable) {
    return cubec_context_create_error(self, "Expression is not assignable");
  }
  if (value->type->kind < CUBEC_TYPE_KIND_INT8 &&
      value->type->kind > CUBEC_TYPE_KIND_UINT64) {
    char *type_name = cubec_context_type_to_string(self, value->type);
    cubec_value_t err = cubec_context_create_error(
        self, "Cannot increment value of type '%s'", type_name);
    cubec_allocator_free(self->allocator, type_name);
    return err;
  }
  switch (value->type->kind) {
  case CUBEC_TYPE_KIND_INT8:
    (*(int8_t *)(value->data))--;
    break;
  case CUBEC_TYPE_KIND_INT16:
    (*(int16_t *)(value->data))--;
    break;
  case CUBEC_TYPE_KIND_INT32:
    (*(int32_t *)(value->data))--;
    break;
  case CUBEC_TYPE_KIND_INT64:
    (*(int64_t *)(value->data))--;
    break;
  case CUBEC_TYPE_KIND_UINT8:
    (*(uint8_t *)(value->data))--;
    break;
  case CUBEC_TYPE_KIND_UINT16:
    (*(uint16_t *)(value->data))--;
    break;
  case CUBEC_TYPE_KIND_UINT32:
    (*(uint32_t *)(value->data))--;
    break;
  case CUBEC_TYPE_KIND_UINT64:
    (*(uint64_t *)(value->data))--;
    break;
  default: {
    char *type_name = cubec_context_type_to_string(self, value->type);
    cubec_value_t err = cubec_context_create_error(
        self, "Cannot increment value of type '%s'", type_name);
    cubec_allocator_free(self->allocator, type_name);
    return err;
  }
  }
  return self->value_undefined;
}
cubec_value_t cubec_context_read_ptr(cubec_context_t ctx, cubec_value_t value) {
  if (value->type->kind != CUBEC_TYPE_KIND_PTR) {
    return cubec_context_create_error(ctx,
                                      "Indirection requires pointer operand");
  }
  cubec_ptr_meta_t ptr = value->type->meta;
  return cubec_context_create_value(ctx, ptr->type, value->is_mutable,
                                    *(void **)value->data, NULL);
}
cubec_value_t cubec_context_write_ptr(cubec_context_t ctx, cubec_value_t ptr,
                                      cubec_value_t value) {
  if (value->type->kind != CUBEC_TYPE_KIND_PTR) {
    return cubec_context_create_error(ctx,
                                      "Indirection requires pointer operand");
  }
  if (!ptr->is_mutable) {
    return cubec_context_create_error(ctx,
                                      "Read-only variable is not assignable");
  }
  cubec_ptr_meta_t meta = ptr->type->meta;
  if (!cubec_context_check_type(ctx, meta->type, value->type)) {
    return cubec_context_create_error(ctx,
                                      "Incompatible pointer types assigning");
  }
  memcpy(*(void **)ptr->data, value->data, meta->type->size);
  return ctx->value_undefined;
}
cubec_value_t cubec_context_to_boolean(cubec_context_t ctx,
                                       cubec_value_t value) {
  switch (value->type->kind) {
  case CUBEC_TYPE_KIND_BOOLEAN:
    return value;
  case CUBEC_TYPE_KIND_INT8:
    return cubec_context_create_boolean(ctx, *(int8_t *)value->data, false,
                                        NULL);
  case CUBEC_TYPE_KIND_INT16:
    return cubec_context_create_boolean(ctx, *(int16_t *)value->data, false,
                                        NULL);
  case CUBEC_TYPE_KIND_INT32:
    return cubec_context_create_boolean(ctx, *(int32_t *)value->data, false,
                                        NULL);
  case CUBEC_TYPE_KIND_INT64:
    return cubec_context_create_boolean(ctx, *(int64_t *)value->data, false,
                                        NULL);
  case CUBEC_TYPE_KIND_UINT8:
    return cubec_context_create_boolean(ctx, *(uint8_t *)value->data, false,
                                        NULL);
  case CUBEC_TYPE_KIND_UINT16:
    return cubec_context_create_boolean(ctx, *(uint16_t *)value->data, false,
                                        NULL);
  case CUBEC_TYPE_KIND_UINT32:
    return cubec_context_create_boolean(ctx, *(uint32_t *)value->data, false,
                                        NULL);
  case CUBEC_TYPE_KIND_UINT64:
    return cubec_context_create_boolean(ctx, *(uint64_t *)value->data, false,
                                        NULL);
  case CUBEC_TYPE_KIND_FLOAT32:
    return cubec_context_create_boolean(ctx, *(float *)value->data, false,
                                        NULL);
  case CUBEC_TYPE_KIND_FLOAT64:
    return cubec_context_create_boolean(ctx, *(double *)value->data, false,
                                        NULL);
  case CUBEC_TYPE_KIND_STR:
    return cubec_context_create_boolean(ctx, true, false, NULL);
  case CUBEC_TYPE_KIND_OPAQUE:
    return cubec_context_create_boolean(ctx, *(void **)value->data, false,
                                        NULL);
  case CUBEC_TYPE_KIND_PTR:
    return cubec_context_create_boolean(ctx, *(void **)value->data, false,
                                        NULL);
  case CUBEC_TYPE_KIND_PTR_ARRAY:
    return cubec_context_create_boolean(ctx, *(void **)value->data, false,
                                        NULL);
  case CUBEC_TYPE_KIND_RESULT:
    return cubec_context_create_boolean(
        ctx, ((cubec_result_data_t)value->data)->flag, false, NULL);
  default:
    break;
  }
  return cubec_context_create_error(ctx, "Cannot convert value to boolean");
}

cubec_value_t cubec_context_plus(cubec_context_t ctx, cubec_value_t value) {
  if (value->type->kind >= 0 && value->type->kind <= CUBEC_TYPE_KIND_FLOAT64) {
    return value;
  }
  return cubec_context_create_error(
      ctx, "Invalid argument type to unary expression");
}
cubec_value_t cubec_context_negtive(cubec_context_t ctx, cubec_value_t value) {
  switch (value->type->kind) {
  case CUBEC_TYPE_KIND_INT8:
    return cubec_context_create_int8(ctx, -*(int8_t *)value->data, false, NULL);
  case CUBEC_TYPE_KIND_INT16:
    return cubec_context_create_int16(ctx, -*(int16_t *)value->data, false,
                                      NULL);
  case CUBEC_TYPE_KIND_INT32:
    return cubec_context_create_int32(ctx, -*(int32_t *)value->data, false,
                                      NULL);
  case CUBEC_TYPE_KIND_INT64:
    return cubec_context_create_int64(ctx, -*(int64_t *)value->data, false,
                                      NULL);
  case CUBEC_TYPE_KIND_UINT8:
    return cubec_context_create_uint8(ctx, -*(uint8_t *)value->data, false,
                                      NULL);
  case CUBEC_TYPE_KIND_UINT16:
    return cubec_context_create_uint16(ctx, -*(uint16_t *)value->data, false,
                                       NULL);
  case CUBEC_TYPE_KIND_UINT32:
    return cubec_context_create_uint32(ctx, -*(uint32_t *)value->data, false,
                                       NULL);
  case CUBEC_TYPE_KIND_UINT64:
    return cubec_context_create_uint64(ctx, -*(uint64_t *)value->data, false,
                                       NULL);
  case CUBEC_TYPE_KIND_FLOAT32:
    return cubec_context_create_float32(ctx, -*(float *)value->data, false,
                                        NULL);
  case CUBEC_TYPE_KIND_FLOAT64:
    return cubec_context_create_float64(ctx, -*(double *)value->data, false,
                                        NULL);
  default:
    break;
  }
  return cubec_context_create_error(
      ctx, "Invalid argument type to unary expression");
}
cubec_value_t cubec_context_typeof(cubec_context_t ctx, cubec_value_t value) {
  return cubec_context_create_type_value(ctx, value->type, false, NULL);
}
cubec_value_t cubec_context_bitwise_not(cubec_context_t ctx,
                                        cubec_value_t value) {
  switch (value->type->kind) {
  case CUBEC_TYPE_KIND_INT8:
    return cubec_context_create_int8(ctx, ~*(int8_t *)value->data, false, NULL);
  case CUBEC_TYPE_KIND_INT16:
    return cubec_context_create_int16(ctx, ~*(int16_t *)value->data, false,
                                      NULL);
  case CUBEC_TYPE_KIND_INT32:
    return cubec_context_create_int32(ctx, ~*(int32_t *)value->data, false,
                                      NULL);
  case CUBEC_TYPE_KIND_INT64:
    return cubec_context_create_int64(ctx, ~*(int64_t *)value->data, false,
                                      NULL);
  case CUBEC_TYPE_KIND_UINT8:
    return cubec_context_create_uint8(ctx, ~*(uint8_t *)value->data, false,
                                      NULL);
  case CUBEC_TYPE_KIND_UINT16:
    return cubec_context_create_uint16(ctx, ~*(uint16_t *)value->data, false,
                                       NULL);
  case CUBEC_TYPE_KIND_UINT32:
    return cubec_context_create_uint32(ctx, ~*(uint32_t *)value->data, false,
                                       NULL);
  case CUBEC_TYPE_KIND_UINT64:
    return cubec_context_create_uint64(ctx, ~*(uint64_t *)value->data, false,
                                       NULL);
  default:
    break;
  }
  return cubec_context_create_error(
      ctx, "Invalid argument type to unary expression");
}
cubec_value_t cubec_context_logical_not(cubec_context_t ctx,
                                        cubec_value_t value) {
  value = cubec_context_to_boolean(ctx, value);
  if (value->type->kind == CUBEC_TYPE_KIND_ERROR) {
    return value;
  }
  return cubec_context_create_boolean(ctx, !*(bool *)value->data, false, NULL);
}