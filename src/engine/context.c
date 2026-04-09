#include "engine/context.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/compare.h"
#include "core/map.h"
#include "core/string.h"
#include "engine/builtin.h"
#include "engine/error.h"
#include "engine/module.h"
#include "engine/numeric.h"
#include "engine/opaque.h"
#include "engine/scope.h"
#include "engine/str.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/void.h"
#include "eval/program.h"
#include "pass/declar_flat.h"
#include "pass/type_fix.h"
#include "writer/context.h"
#include "writer/program.h"
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
struct _cubec_context_t {
  cubec_allocator_t allocator;

  cubec_map_t modules;
  cubec_array_t types;
  cubec_array_t strings;

  cubec_type_t type_type;

  cubec_value_t value_undefined;

  cubec_scope_t root;
  cubec_scope_t scope;
  cubec_value_t self;
};

static void cubec_context_dispose(cubec_context_t self,
                                  cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->types);
  cubec_allocator_free(allocator, self->modules);
  cubec_allocator_free(allocator, self->root);
  cubec_allocator_free(allocator, self->strings);
}

static char *cubec_type_type_to_string(cubec_type_t self,
                                       cubec_allocator_t allocator) {
  return cubec_create_cstring(allocator, "type");
}

static void cubec_init_type_type(cubec_context_t self) {
  struct _cubec_type_operator_t opt = {
      .type_to_string = cubec_type_type_to_string,
  };
  cubec_type_t type_type = cubec_create_type(
      self->allocator, CUBEC_VALUE_TYPE_TYPE, sizeof(cubec_type_t *),
      sizeof(cubec_type_t *), NULL, &opt);
  cubec_context_create_value(self, type_type, false, &type_type, "type");
  cubec_array_push(self->types, type_type);
  self->type_type = type_type;
}

static void cubec_init_any_type(cubec_context_t self) {
  cubec_context_create_type(self, CUBEC_VALUE_TYPE_ANY, 0, 0, NULL, NULL,
                            "any");
}

static void cubec_context_init_type(cubec_context_t self) {
  cubec_init_type_type(self);
  cubec_init_any_type(self);
  cubec_init_void_type(self);
  cubec_init_numeric_type(self);
  cubec_init_error_type(self);
  cubec_init_builtin_type(self);
  cubec_init_opaque_type(self);
  cubec_init_str_type(self);
}

static void cubec_context_init_value(cubec_context_t self) {
  cubec_value_t vtype = cubec_context_load(self, "void");
  cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
  self->value_undefined =
      cubec_context_create_value(self, type, false, NULL, NULL);
}

static cubec_ast_node_t cubec_context_visit_node(cubec_allocator_t allocator,
                                                 cubec_ast_node_t node,
                                                 cubec_context_t ctx) {
  node = cubec_pass_declar_flat(allocator, node, ctx);
  if (node->changed || node->type == CUBEC_NODE_TYPE_ERROR) {
    return node;
  }
  node = cubec_pass_type_fix(allocator, node, ctx);
  if (node->changed || node->type == CUBEC_NODE_TYPE_ERROR) {
    return node;
  }
  return node;
}

cubec_context_t cubec_create_context(cubec_allocator_t allocator) {
  cubec_context_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_context_t),
                            (cubec_dispose_fn_t)cubec_context_dispose);
  self->allocator = allocator;
  self->root = cubec_create_scope(allocator, NULL);
  self->scope = self->root;
  cubec_array_initialize_t types_initialize = {
      .autofree = true,
  };
  self->types = cubec_create_array(allocator, &types_initialize);
  cubec_array_initialize_t strings_initialize = {
      .autofree = true,
  };
  self->strings = cubec_create_array(allocator, &strings_initialize);
  cubec_map_initialize_t module_initialize = {
      .autofree_value = true,
      .autofree_key = false,
      .compare = (cubec_compare_fn_t)strcmp,
  };
  self->modules = cubec_create_map(allocator, &module_initialize);
  cubec_context_init_type(self);
  cubec_context_init_value(self);
  self->self = NULL;
  return self;
}

cubec_value_t cubec_context_load_module(cubec_context_t self,
                                        const char *filename) {
  cubec_module_t module = cubec_map_get(self->modules, filename, NULL);
  if (!module) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
      return cubec_create_error(self, "File '%s' is not exist", filename);
    }
    fseek(fp, 0, SEEK_END);
    size_t len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *source = cubec_allocator_alloc(self->allocator, len + 1, NULL);
    fread(source, len, 1, fp);
    source[len] = 0;
    fclose(fp);
    cubec_ast_node_t node =
        cubec_read_ast_node(self->allocator, filename, source, self,
                            (cubec_visit_ast_fn_t)cubec_context_visit_node);
    if (node->type == CUBEC_NODE_TYPE_ERROR) {
      cubec_ast_error_t error = (cubec_ast_error_t)node;
      cubec_value_t res =
          cubec_create_compile_error(self, node, error->message);
      cubec_allocator_free(self->allocator, node);
      cubec_allocator_free(self->allocator, source);
      return res;
    }
    cubec_value_t value = cubec_eval_program(self, node);
    if (cubec_value_is_error(value)) {
      cubec_allocator_free(self->allocator, node);
      cubec_allocator_free(self->allocator, source);
      return value;
    }
    module =
        cubec_create_module(self->allocator, node, filename, source, value);
    cubec_map_set(self->modules, (void *)cubec_module_get_filename(module),
                  module, NULL);
  }
  return cubec_module_get_value(module);
}

cubec_value_t cubec_context_write_module(cubec_context_t self,
                                         const char *filename,
                                         const char *dst_filename) {
  cubec_module_t module = cubec_map_get(self->modules, filename, NULL);
  if (!module) {
    return cubec_create_error(self, "module %s is not loaded", filename);
  }
  FILE *fp = fopen(dst_filename, "w");
  cubec_write_context ctx = {
      .allocator = self->allocator,
      .indent = 0,
  };
  cubec_ast_node_t node = cubec_module_get_node(module);
  cubec_write_program(fp, node, &ctx);
  fclose(fp);
  return self->value_undefined;
}

cubec_value_t cubec_context_create_type(cubec_context_t self,
                                        cubec_type_kind_t kind, size_t size,
                                        size_t align, void *meta,
                                        cubec_type_operator_t opt,
                                        const char *name) {
  cubec_type_t type =
      cubec_create_type(self->allocator, kind, size, align, meta, opt);
  cubec_array_push(self->types, type);
  return cubec_context_create_value(self, self->type_type, false, &type, name);
}

cubec_value_t cubec_context_create_value(cubec_context_t self,
                                         cubec_type_t type, bool mutable,
                                         void *data, const char *name) {
  cubec_value_t value =
      cubec_create_value(self->allocator, type, mutable, data);
  cubec_scope_store(self->scope, self->allocator, value, name);
  return value;
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
  if (self->self) {
    cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(self->self);
    return cubec_struct_type_get_attribute(type, name);
  }
  return cubec_create_error(self, "use of undeclared identifier '%s'", name);
}
cubec_type_t cubec_context_load_type(cubec_context_t self, const char *name) {
  cubec_value_t vtype = cubec_context_load(self, name);
  if (vtype) {
    cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(vtype);
    return type;
  }
  return NULL;
}
cubec_value_t cubec_context_declar(cubec_context_t self, const char *name,
                                   cubec_value_t value) {
  if (cubec_scope_load(self->scope, name)) {
    return cubec_create_error(self, "Duplicate variable declaration");
  }
  if (self->self) {
    cubec_type_t stru = *(cubec_type_t *)cubec_value_get_data(self->self);
    cubec_struct_type_add_attribute(stru, self->allocator, name, value);
  } else {
    cubec_scope_store(self->scope, self->allocator, value, name);
  }
  return value;
}
cubec_value_t cubec_context_set_self(cubec_context_t self,
                                     cubec_value_t value) {
  cubec_value_t old = self->self;
  self->self = value;
  return old;
}
cubec_value_t cubec_context_get_self(cubec_context_t self) {
  return self->self;
}
char *const cubec_context_create_cstring(cubec_context_t self,
                                         const char *src) {
  char *str = cubec_create_cstring(self->allocator, src);
  cubec_array_push(self->strings, str);
  return str;
}

cubec_value_t cubec_context_get_undefined(cubec_context_t self) {
  return self->value_undefined;
}

cubec_allocator_t cubec_context_get_allocator(cubec_context_t self) {
  return self->allocator;
}