#include "engine/context.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/string.h"
#include "engine/boolean.h"
#include "engine/builtin.h"
#include "engine/error.h"
#include "engine/module.h"
#include "engine/numeric.h"
#include "engine/opaque.h"
#include "engine/ptr.h"
#include "engine/scope.h"
#include "engine/str.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/union.h"
#include "engine/value.h"
#include "engine/void.h"
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

struct _cubec_context_t {
  cubec_allocator_t allocator;

  cubec_hash_map_t modules;
  cubec_array_t types;
  cubec_array_t strings;

  cubec_type_t type_type;
  cubec_type_t interrupt_type;

  cubec_value_t value_undefined;

  cubec_scope_t root;
  cubec_scope_t scope;

  cubec_static_scope_t static_scope;
  bool comptime;
};

static void cubec_context_dispose(cubec_context_t self,
                                  cubec_allocator_t allocator) {
  while (self->static_scope) {
    cubec_context_pop_static_scope(self);
  }
  while (self->scope != self->root) {
    cubec_context_pop_scope(self);
  }
  cubec_allocator_free(allocator, self->types);
  cubec_allocator_free(allocator, self->modules);
  cubec_allocator_free(allocator, self->root);
  cubec_allocator_free(allocator, self->strings);
}

static char *cubec_type_type_to_string(cubec_type_t self,
                                       cubec_allocator_t allocator) {
  return cubec_create_cstring(allocator, "type");
}

static cubec_value_t cubec_type_value_to_string(cubec_value_t self,
                                                cubec_context_t ctx) {
  cubec_type_t *type = (cubec_type_t *)cubec_value_get_data(self);
  if (!type) {
    return cubec_create_str(ctx, "type{<runtime>}", NULL);
  } else {
    cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
    char *s = cubec_type_to_string(*type, allocator);
    cubec_value_t res = cubec_create_str(ctx, s, NULL);
    cubec_allocator_free(allocator, s);
    return res;
  }
}

static cubec_value_t cubec_type_value_unref(cubec_value_t self,
                                            cubec_context_t ctx) {
  cubec_type_t *type = (cubec_type_t *)cubec_value_get_data(self);
  if (!type) {
    return cubec_create_str(ctx, "type{<runtime>}", NULL);
  } else {
    return cubec_create_ptr_type(ctx, *type, true, false);
  }
}
static cubec_value_t cubec_type_value_get_field(cubec_value_t self,
                                                cubec_context_t ctx,
                                                const char *name) {
  cubec_type_t *type = (cubec_type_t *)cubec_value_get_data(self);
  if (type) {
    cubec_type_t t = *type;
    if (cubec_type_get_kind(t) == CUBEC_VALUE_TYPE_STRUCT) {
      cubec_value_t val = cubec_struct_type_get_attribute(t, name);
      if (!val) {
        return cubec_create_error(ctx, "no member named '%s' in value", name);
      }
      return cubec_context_create_value(ctx, cubec_value_get_type(val), false,
                                        cubec_value_get_data(val), NULL);
    }
  }
  return cubec_create_error(ctx, "value does not support member access");
}
static cubec_value_t cubec_type_value_set_field(cubec_value_t self,
                                                cubec_context_t ctx,
                                                const char *name,
                                                cubec_value_t value) {
  cubec_type_t *type = (cubec_type_t *)cubec_value_get_data(self);
  if (type) {
    cubec_type_t t = *type;
    if (cubec_type_get_kind(t) == CUBEC_VALUE_TYPE_STRUCT) {
      cubec_value_t val = cubec_struct_type_get_attribute(t, name);
      if (!val) {
        return cubec_create_error(ctx, "no member named '%s' in value", name);
      }
      return cubec_value_assigment(val, ctx, value);
    }
  }
  return cubec_create_error(ctx, "value does not support member access");
}

static void cubec_init_type_type(cubec_context_t self) {
  struct _cubec_type_operator_t opt = {
      .type_to_string = cubec_type_type_to_string,
      .to_string = cubec_type_value_to_string,
      .unref = cubec_type_value_unref,
      .get_field = cubec_type_value_get_field,
      .set_field = cubec_type_value_set_field,
  };
  cubec_type_t type_type = cubec_create_type(
      self->allocator, CUBEC_VALUE_TYPE_TYPE, sizeof(cubec_type_t *),
      sizeof(cubec_type_t *), NULL, &opt);
  cubec_context_create_value(self, type_type, false, &type_type, "type");
  cubec_array_push(self->types, type_type);
  self->type_type = type_type;
  self->interrupt_type =
      cubec_create_type(self->allocator, CUBEC_VALUE_TYPE_INTERRUPT,
                        sizeof(void *), sizeof(void *), NULL, NULL);
  cubec_array_push(self->types, self->interrupt_type);
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
  cubec_init_boolean_type(self);
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

cubec_context_t cubec_create_context(cubec_allocator_t allocator) {
  cubec_context_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_context_t),
                            (cubec_dispose_fn_t)cubec_context_dispose);
  self->allocator = allocator;
  self->root = cubec_create_scope(allocator, NULL);
  self->scope = self->root;
  self->static_scope = NULL;
  self->comptime = false;
  cubec_array_initialize_t types_initialize = {
      .autofree = true,
  };
  self->types = cubec_create_array(allocator, &types_initialize);
  cubec_array_initialize_t strings_initialize = {
      .autofree = true,
  };
  self->strings = cubec_create_array(allocator, &strings_initialize);
  cubec_hash_map_initialize_t module_initialize = {
      .autofree_value = true,
      .autofree_key = false,
      .hash = (cubec_hash_fn_t)cubec_cstring_sdb,
      .compare = (cubec_compare_fn_t)strcmp,
  };
  self->modules = cubec_create_hash_map(allocator, &module_initialize);
  cubec_context_init_type(self);
  cubec_context_init_value(self);
  return self;
}

cubec_value_t cubec_context_load_module(cubec_context_t self,
                                        const char *filename) {
  cubec_module_t module =
      cubec_hash_map_get(self->modules, filename, NULL, NULL);
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
        cubec_read_ast_node(self->allocator, filename, source, self);
    if (node->type == CUBEC_NODE_TYPE_ERROR) {
      cubec_ast_error_t error = (cubec_ast_error_t)node;
      cubec_value_t res =
          cubec_create_compile_error(self, node, error->message);
      cubec_allocator_free(self->allocator, node);
      cubec_allocator_free(self->allocator, source);
      return res;
    }
    cubec_value_t value = self->value_undefined;
    if (cubec_value_type_is(value, CUBEC_VALUE_TYPE_ERROR)) {
      cubec_allocator_free(self->allocator, node);
      cubec_allocator_free(self->allocator, source);
      return value;
    }
    module =
        cubec_create_module(self->allocator, node, filename, source, value);
    cubec_hash_map_set(self->modules, (void *)cubec_module_get_filename(module),
                       module, NULL, NULL);
  }
  return cubec_module_get_value(module);
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
cubec_value_t cubec_context_create_interrupt(cubec_context_t self,
                                             cubec_value_t value) {
  return cubec_context_create_value(self, self->interrupt_type, false, &value,
                                    NULL);
}

cubec_value_t cubec_context_create_value(cubec_context_t self,
                                         cubec_type_t type, bool mutable,
                                         void *data, const char *name) {
  cubec_value_t value =
      cubec_create_value(self->allocator, type, mutable, data);
  if (name) {
    return cubec_context_declar(self, name, value);
  } else {
    cubec_scope_store(self->scope, self->allocator, value, name);
  }
  return value;
}

cubec_value_t cubec_context_load(cubec_context_t self, const char *name) {
  if (strcmp(name, "true") == 0) {
    return cubec_create_boolean(self, true, false, NULL);
  }
  if (strcmp(name, "false") == 0) {
    return cubec_create_boolean(self, false, false, NULL);
  }
  if (strcmp(name, "__self__") == 0) {
    cubec_static_scope_t scope = self->static_scope;
    while (scope) {
      if (cubec_value_type_is(scope->binding, CUBEC_VALUE_TYPE_TYPE)) {
        return scope->binding;
      }
      scope = scope->parent;
    }
  }
  cubec_scope_t scope = self->scope;
  while (scope) {
    cubec_value_t value = cubec_scope_load(scope, name);
    if (value) {
      return value;
    }
    scope = cubec_scope_get_parent(scope);
  }
  if (self->static_scope) {
    cubec_value_t static_scope = self->static_scope->binding;
    cubec_type_t ctx_type = cubec_value_get_type(static_scope);
    if (cubec_type_get_kind(ctx_type) == CUBEC_VALUE_TYPE_TYPE) {
      cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(static_scope);
      if (cubec_type_get_kind(type) == CUBEC_VALUE_TYPE_STRUCT) {
        return cubec_struct_type_get_attribute(type, name);
      } else if (cubec_type_get_kind(type) == CUBEC_VALUE_TYPE_UNION) {
        return cubec_union_type_get_attribute(type, name);
      }
    }
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
  if (self->static_scope) {
    cubec_value_t static_scope = self->static_scope->binding;
    cubec_type_t ctx_type = cubec_value_get_type(static_scope);
    if (cubec_type_get_kind(ctx_type) == CUBEC_VALUE_TYPE_TYPE) {
      cubec_type_t type = *(cubec_type_t *)cubec_value_get_data(static_scope);
      if (cubec_type_get_kind(type) == CUBEC_VALUE_TYPE_STRUCT) {
        cubec_struct_type_add_attribute(type, self->allocator, name, value);
      } else if (cubec_type_get_kind(type) == CUBEC_VALUE_TYPE_UNION) {
        cubec_union_type_add_attribute(type, self->allocator, name, value);
      }
    }
  }
  cubec_scope_store(self->scope, self->allocator, value, name);
  return value;
}
void cubec_context_push_static_scope(cubec_context_t self,
                                     cubec_value_t value) {
  cubec_static_scope_t scope = cubec_allocator_alloc(
      self->allocator, sizeof(struct _cubec_static_scope_t), NULL);
  scope->parent = self->static_scope;
  scope->binding = value;
  self->static_scope = scope;
}

void cubec_context_pop_static_scope(cubec_context_t self) {
  cubec_static_scope_t scope = self->static_scope;
  self->static_scope = scope->parent;
  cubec_allocator_free(self->allocator, scope);
}
cubec_static_scope_t cubec_context_get_static_scope(cubec_context_t self) {
  return self->static_scope;
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
bool cubec_context_is_comptime(cubec_context_t ctx) { return ctx->comptime; }

void cubec_context_set_comptime(cubec_context_t ctx, bool comptime) {
  ctx->comptime = comptime;
}
void cubec_context_push_scope(cubec_context_t self) {
  self->scope = cubec_create_scope(self->allocator, self->scope);
}
void cubec_context_pop_scope(cubec_context_t self) {
  cubec_scope_t scope = self->scope;
  self->scope = cubec_scope_get_parent(self->scope);
  cubec_allocator_free(self->allocator, scope);
}
cubec_scope_t cubec_context_get_scope(cubec_context_t self) {
  return self->scope;
}
void cubec_context_set_scope(cubec_context_t self, cubec_scope_t scope) {
  self->scope = scope;
}