#include "engine/context.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/compare.h"
#include "core/map.h"
#include "engine/module.h"
#include "engine/scope.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
struct _cubec_context_t {
  cubec_allocator_t allocator;

  cubec_array_t visits;

  cubec_map_t modules;
  cubec_array_t types;

  cubec_type_t error_type;
  cubec_type_t any_type;
  cubec_type_t void_type;
  cubec_type_t type_type;
  cubec_type_t module_type;
  cubec_type_t bool_type;
  cubec_type_t int8_type;
  cubec_type_t int16_type;
  cubec_type_t int32_type;
  cubec_type_t int64_type;
  cubec_type_t uint8_type;
  cubec_type_t uint16_type;
  cubec_type_t uint32_type;
  cubec_type_t uint64_type;
  cubec_type_t float16_type;
  cubec_type_t float32_type;
  cubec_type_t float64_type;
  cubec_type_t str_type;
  cubec_type_t opaque_type;

  cubec_scope_t root;
  cubec_scope_t scope;
};

static void cubec_context_dispose(cubec_context_t self,
                                  cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->types);
  cubec_allocator_free(allocator, self->modules);
  cubec_allocator_free(allocator, self->root);
  cubec_allocator_free(allocator, self->visits);
}

static void cubec_context_init_type(cubec_context_t self) {
  self->void_type =
      cubec_context_create_type(self, CUBEC_VALUE_TYPE_VOID, 0, NULL);
  self->any_type =
      cubec_context_create_type(self, CUBEC_VALUE_TYPE_ANY, 0, NULL);
  self->error_type = cubec_context_create_type(self, CUBEC_VALUE_TYPE_ERROR,
                                               sizeof(const char **), NULL);
  self->error_type = cubec_context_create_type(
      self, CUBEC_VALUE_TYPE_MODULE, sizeof(const cubec_module_t *), NULL);
  self->type_type = cubec_context_create_type(
      self, CUBEC_VALUE_TYPE_TYPE, sizeof(const cubec_type_t *), NULL);

  self->bool_type = cubec_context_create_type(self, CUBEC_VALUE_TYPE_BOOL,
                                              sizeof(bool), NULL);
  self->int8_type = cubec_context_create_type(self, CUBEC_VALUE_TYPE_INT8,
                                              sizeof(int8_t), NULL);
  self->int16_type = cubec_context_create_type(self, CUBEC_VALUE_TYPE_INT16,
                                               sizeof(int16_t), NULL);
  self->int32_type = cubec_context_create_type(self, CUBEC_VALUE_TYPE_INT32,
                                               sizeof(int32_t), NULL);
  self->int64_type = cubec_context_create_type(self, CUBEC_VALUE_TYPE_INT64,
                                               sizeof(int64_t), NULL);
  self->uint8_type = cubec_context_create_type(self, CUBEC_VALUE_TYPE_UINT8,
                                               sizeof(uint8_t), NULL);
  self->uint16_type = cubec_context_create_type(self, CUBEC_VALUE_TYPE_UINT16,
                                                sizeof(uint16_t), NULL);
  self->uint32_type = cubec_context_create_type(self, CUBEC_VALUE_TYPE_UINT32,
                                                sizeof(uint32_t), NULL);
  self->uint64_type = cubec_context_create_type(self, CUBEC_VALUE_TYPE_UINT64,
                                                sizeof(uint64_t), NULL);
  self->float16_type = cubec_context_create_type(self, CUBEC_VALUE_TYPE_FLOAT16,
                                                 sizeof(_Float16), NULL);
  self->float32_type = cubec_context_create_type(self, CUBEC_VALUE_TYPE_FLOAT32,
                                                 sizeof(float), NULL);
  self->float64_type = cubec_context_create_type(self, CUBEC_VALUE_TYPE_FLOAT64,
                                                 sizeof(double), NULL);
  self->str_type = cubec_context_create_type(self, CUBEC_VALUE_TYPE_STR,
                                             sizeof(const char **), NULL);
  self->opaque_type = cubec_context_create_type(self, CUBEC_VALUE_TYPE_OPAQUE,
                                                sizeof(void *), NULL);
}

static void cubec_context_init_value(cubec_context_t self) {
  cubec_context_create_value(self, self->void_type, false, NULL, "undefined");
  cubec_context_create_value(self, self->type_type, false, &self->type_type,
                             "type");
  cubec_context_create_value(self, self->type_type, false, &self->void_type,
                             "void");
  cubec_context_create_value(self, self->type_type, false, &self->bool_type,
                             "bool");
  cubec_context_create_value(self, self->type_type, false, &self->int8_type,
                             "i8");
  cubec_context_create_value(self, self->type_type, false, &self->int16_type,
                             "i16");
  cubec_context_create_value(self, self->type_type, false, &self->int16_type,
                             "i32");
  cubec_context_create_value(self, self->type_type, false, &self->int32_type,
                             "i64");
  cubec_context_create_value(self, self->type_type, false, &self->uint8_type,
                             "u8");
  cubec_context_create_value(self, self->type_type, false, &self->uint16_type,
                             "u16");
  cubec_context_create_value(self, self->type_type, false, &self->uint16_type,
                             "u32");
  cubec_context_create_value(self, self->type_type, false, &self->uint32_type,
                             "u64");
  cubec_context_create_value(self, self->type_type, false, &self->float16_type,
                             "f16");
  cubec_context_create_value(self, self->type_type, false, &self->float32_type,
                             "f32");
  cubec_context_create_value(self, self->type_type, false, &self->float64_type,
                             "f64");
}

cubec_context_t cubec_create_context(cubec_allocator_t allocator) {
  cubec_context_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_context_t),
                            (cubec_dispose_fn_t)cubec_context_dispose);
  self->allocator = allocator;
  self->visits = cubec_create_array(allocator, NULL);
  self->root = cubec_create_scope(allocator, NULL);
  self->scope = self->root;
  cubec_array_initialize_t types_initialize = {
      .autofree = true,
  };
  self->types = cubec_create_array(allocator, &types_initialize);
  cubec_context_init_type(self);
  cubec_context_init_value(self);
  cubec_map_initialize_t module_initialize = {
      .autofree_value = true,
      .autofree_key = false,
      .compare = (cubec_compare_fn_t)strcmp,
  };
  self->modules = cubec_create_map(allocator, &module_initialize);
  return self;
}

void cubec_context_add_visit(cubec_context_t self, cubec_visit_ast_fn_t visit) {
  cubec_array_push(self->visits, visit);
}

cubec_value_t cubec_context_load_module(cubec_context_t self,
                                        const char *filename,
                                        const char *name) {
  cubec_module_t module = cubec_map_get(self->modules, filename, NULL);
  if (!module) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
      return cubec_context_create_error(self, "File '%s' is not exist",
                                        filename);
    }
    fseek(fp, 0, SEEK_END);
    size_t len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char source[len + 1];
    fread(source, len, 1, fp);
    source[len] = 0;
    fclose(fp);
    size_t num_visits = cubec_array_get_size(self->visits);
    cubec_visit_ast_fn_t *visits = cubec_array_get_data(self->visits);
    cubec_ast_node_t node = cubec_read_ast_node(
        self->allocator, filename, source, self, num_visits, visits);
    if (node->type == CUBEC_NODE_TYPE_ERROR) {
      cubec_ast_error_t error = (cubec_ast_error_t)node;
      cubec_value_t res =
          cubec_context_create_error(self, "%s", error->message);
      cubec_allocator_free(self->allocator, node);
      return res;
    }
    module = cubec_create_module(self->allocator, node, filename, self->root);
    cubec_map_set(self->modules, (void *)cubec_module_get_filename(module),
                  module, NULL);
  }
  return cubec_context_create_value(self, self->module_type, false, &module,
                                    name);
}

cubec_type_t cubec_context_create_type(cubec_context_t self,
                                       cubec_type_kind_t kind, size_t size,
                                       void *meta) {
  cubec_type_t type = cubec_create_type(self->allocator, kind, size, meta);
  cubec_array_push(self->types, type);
  return type;
}

cubec_value_t cubec_context_create_value(cubec_context_t self,
                                         cubec_type_t type, bool mutable,
                                         void *data, const char *name) {
  cubec_value_t value =
      cubec_create_value(self->allocator, type, mutable, data);
  cubec_scope_store(self->scope, self->allocator, value, name);
  return value;
}

cubec_value_t cubec_context_create_error(cubec_context_t self, const char *fmt,
                                         ...) {
  va_list args;
  va_start(args, fmt);
  size_t len = vsnprintf(NULL, 0, fmt, args);
  va_end(args);
  char message[len + 1];
  va_start(args, fmt);
  vsprintf(message, fmt, args);
  va_end(args);
  return cubec_context_create_value(self, self->error_type, false, &message,
                                    NULL);
}
cubec_value_t cubec_context_load(cubec_context_t self, const char *name) {
  cubec_scope_t scope = self->scope;
  while (scope) {
    cubec_value_t value = cubec_scope_load(scope, name);
    if (value) {
      return value;
    }
    scope = cubec_scope_get_parent(scope);
  }
  return NULL;
}

cubec_type_t cubec_context_create_struct_type(cubec_context_t self,
                                              size_t align, const char *name) {
  cubec_struct_meta_t meta =
      cubec_create_struct_meta(self->allocator, align, name);
  cubec_type_t type =
      cubec_context_create_type(self, CUBEC_VALUE_TYPE_STRUCT, 1, meta);
  return type;
}