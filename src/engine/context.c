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
#include "engine/scope.h"
#include "engine/str.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/union.h"
#include "engine/value.h"
#include "engine/void.h"
#include "resolve/program.h"
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct _context_t {
  allocator_t allocator;

  hash_map_t modules;
  array_t types;
  array_t strings;

  type_t type_type;
  type_t interrupt_type;
  type_t null_type;

  value_t value_undefined;
  value_t value_null;

  scope_t root;
  scope_t scope;

  static_scope_t static_scope;
  bool comptime;
};

static void context_dispose(context_t self, allocator_t allocator) {
  while (self->static_scope) {
    context_pop_static_scope(self);
  }
  while (self->scope != self->root) {
    context_pop_scope(self);
  }
  allocator_free(allocator, self->types);
  allocator_free(allocator, self->modules);
  allocator_free(allocator, self->root);
  allocator_free(allocator, self->strings);
}

static char *type_type_to_string(type_t self, allocator_t allocator) {
  return create_cstring(allocator, "type");
}

static value_t type_value_to_string(value_t self, context_t ctx) {
  type_t *type = (type_t *)value_get_data(self);
  if (!type) {
    return create_str(ctx, "type{<runtime>}", NULL);
  } else {
    allocator_t allocator = context_get_allocator(ctx);
    char *s = type_to_string(*type, allocator);
    value_t res = create_str(ctx, s, NULL);
    allocator_free(allocator, s);
    return res;
  }
}

static value_t type_value_unref(value_t self, context_t ctx) {
  type_t type = *(type_t *)value_get_data(self);
  return create_type_value(ctx, type_get_ptr_type(type, ctx), false, NULL);
}
static value_t type_value_get_field(value_t self, context_t ctx,
                                    const char *name) {
  type_t *type = (type_t *)value_get_data(self);
  if (type) {
    type_t t = *type;
    if (type_get_kind(t) == VALUE_TYPE_STRUCT) {
      value_t val = struct_type_get_attribute(t, name);
      if (!val) {
        return create_error(ctx, "no member named '%s' in value", name);
      }
      return context_create_value(ctx, value_get_type(val), false,
                                  value_get_data(val), NULL);
    }
  }
  return create_error(ctx, "value does not support member access");
}
static value_t type_value_set_field(value_t self, context_t ctx,
                                    const char *name, value_t value) {
  type_t *type = (type_t *)value_get_data(self);
  if (type) {
    type_t t = *type;
    if (type_get_kind(t) == VALUE_TYPE_STRUCT) {
      value_t val = struct_type_get_attribute(t, name);
      if (!val) {
        return create_error(ctx, "no member named '%s' in value", name);
      }
      return value_assigment(val, ctx, value);
    }
  }
  return create_error(ctx, "value does not support member access");
}

static void init_type_type(context_t self) {
  struct _type_operator_t opt = {
      .type_to_string = type_type_to_string,
      .to_string = type_value_to_string,
      .unref = type_value_unref,
      .get_field = type_value_get_field,
      .set_field = type_value_set_field,
  };
  type_t type_type =
      create_type(self->allocator, VALUE_TYPE_TYPE, sizeof(type_t *),
                  sizeof(type_t *), NULL, &opt);
  context_create_value(self, type_type, false, &type_type, "type");
  array_push(self->types, type_type);
  self->type_type = type_type;
}

static value_t null_convert(value_t self, context_t ctx, type_t type) {
  if (type_get_kind(type) == VALUE_TYPE_PTR ||
      type_get_kind(type) == VALUE_TYPE_PARRAY ||
      type_get_kind(type) == VALUE_TYPE_OPAQUE) {
    void *data = NULL;
    value_t val = context_create_value(ctx, type, false, &data, NULL);
    value_set_comptime(val, true);
    return val;
  }
  allocator_t allocator = context_get_allocator(ctx);
  char *dst_type_name = type_to_string(type, allocator);
  value_t err = create_error(ctx, "cannot convert null to '%s'", dst_type_name);
  allocator_free(allocator, dst_type_name);
  return err;
}

static void context_init_type(context_t self) {
  init_type_type(self);
  self->interrupt_type =
      create_type(self->allocator, VALUE_TYPE_INTERRUPT, sizeof(void *),
                  sizeof(void *), NULL, NULL);
  array_push(self->types, self->interrupt_type);
  context_create_type(self, VALUE_TYPE_ANY, 0, 0, NULL, NULL, "any");
  struct _type_operator_t opt = {
      .convert = null_convert,
  };
  self->null_type =
      create_type(self->allocator, VALUE_TYPE_NULL, 0, 0, NULL, &opt);
  array_push(self->types, self->null_type);
  init_void_type(self);
  init_numeric_type(self);
  init_boolean_type(self);
  init_error_type(self);
  init_builtin_type(self);
  init_opaque_type(self);
  init_str_type(self);
}

static void context_init_value(context_t self) {
  value_t vtype = context_load(self, "void");
  type_t type = *(type_t *)value_get_data(vtype);
  self->value_undefined = context_create_value(self, type, false, NULL, NULL);
  value_set_comptime(self->value_undefined, true);
  self->value_null =
      context_create_value(self, self->null_type, false, NULL, NULL);
  value_set_comptime(self->value_null, true);
}

context_t create_context(allocator_t allocator) {
  context_t self = allocator_alloc(allocator, sizeof(struct _context_t),
                                   (dispose_fn_t)context_dispose);
  self->allocator = allocator;
  self->root = create_scope(allocator, NULL);
  self->scope = self->root;
  self->static_scope = NULL;
  self->comptime = false;
  array_initialize_t types_initialize = {
      .autofree = true,
  };
  self->types = create_array(allocator, &types_initialize);
  array_initialize_t strings_initialize = {
      .autofree = true,
  };
  self->strings = create_array(allocator, &strings_initialize);
  hash_map_initialize_t module_initialize = {
      .autofree_value = true,
      .autofree_key = false,
      .hash = (hash_fn_t)cstring_sdb,
      .compare = (compare_fn_t)strcmp,
  };
  self->modules = create_hash_map(allocator, &module_initialize);
  context_init_type(self);
  context_init_value(self);
  return self;
}

value_t context_load_module(context_t self, const char *filename) {
  module_t module = hash_map_get(self->modules, filename, NULL, NULL);
  if (!module) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
      return create_error(self, "File '%s' is not exist", filename);
    }
    fseek(fp, 0, SEEK_END);
    size_t len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char *source = allocator_alloc(self->allocator, len + 1, NULL);
    fread(source, len, 1, fp);
    source[len] = 0;
    fclose(fp);
    ast_node_t node = read_ast_node(self->allocator, filename, source, self);
    if (node->type == NODE_TYPE_ERROR) {
      ast_error_t error = (ast_error_t)node;
      value_t res = create_compile_error(self, node, error->message);
      allocator_free(self->allocator, node);
      allocator_free(self->allocator, source);
      return res;
    }
    value_t value = resolve_program(self, node);
    if (value_type_is(value, VALUE_TYPE_ERROR)) {
      allocator_free(self->allocator, node);
      allocator_free(self->allocator, source);
      return value;
    }
    module = create_module(self->allocator, node, filename, source, value);
    hash_map_set(self->modules, (void *)module_get_filename(module), module,
                 NULL, NULL);
  }
  return module_get_value(module);
}

value_t context_create_type(context_t self, type_kind_t kind, size_t size,
                            size_t align, void *meta, type_operator_t opt,
                            const char *name) {
  type_t type = create_type(self->allocator, kind, size, align, meta, opt);
  array_push(self->types, type);
  value_t value =
      context_create_value(self, self->type_type, false, &type, name);
  value_set_comptime(value, true);
  return value;
}
value_t context_create_interrupt(context_t self, value_t value) {
  return context_create_value(self, self->interrupt_type, false, &value, NULL);
}

value_t context_create_value(context_t self, type_t type, bool mutable,
                             void *data, const char *name) {
  value_t value = create_value(self->allocator, type, mutable, data);
  if (name) {
    return context_declar(self, name, value);
  } else {
    scope_store(self->scope, self->allocator, value, name);
  }
  return value;
}

value_t context_load(context_t self, const char *name) {
  if (strcmp(name, "true") == 0) {
    value_t val = create_boolean(self, true, false, NULL);
    value_set_comptime(val, true);
    return val;
  }
  if (strcmp(name, "false") == 0) {
    value_t val = create_boolean(self, false, false, NULL);
    value_set_comptime(val, true);
    return val;
  }
  if (strcmp(name, "null") == 0) {
    return self->value_null;
  }
  if (strcmp(name, "__self__") == 0) {
    static_scope_t scope = self->static_scope;
    while (scope) {
      if (value_type_is(scope->binding, VALUE_TYPE_TYPE)) {
        return scope->binding;
      }
      scope = scope->parent;
    }
  }
  scope_t scope = self->scope;
  while (scope) {
    value_t value = scope_load(scope, name);
    if (value) {
      return value;
    }
    scope = scope_get_parent(scope);
  }
  return create_error(self, "use of undeclared identifier '%s'", name);
}
type_t context_load_type(context_t self, const char *name) {
  value_t vtype = context_load(self, name);
  if (vtype) {
    type_t type = *(type_t *)value_get_data(vtype);
    return type;
  }
  return NULL;
}
value_t context_declar(context_t self, const char *name, value_t value) {
  if (scope_load(self->scope, name)) {
    return create_error(self, "Duplicate variable declaration");
  }
  if (self->static_scope) {
    value_t static_scope = self->static_scope->binding;
    type_t ctx_type = value_get_type(static_scope);
    if (type_get_kind(ctx_type) == VALUE_TYPE_TYPE) {
      type_t type = *(type_t *)value_get_data(static_scope);
      if (type_get_kind(type) == VALUE_TYPE_STRUCT) {
        struct_type_add_attribute(type, self->allocator, name, value);
      } else if (type_get_kind(type) == VALUE_TYPE_UNION) {
        union_type_add_attribute(type, self->allocator, name, value);
      }
    }
  }
  scope_store(self->scope, self->allocator, value, name);
  return value;
}
void context_push_static_scope(context_t self, value_t value) {
  static_scope_t scope =
      allocator_alloc(self->allocator, sizeof(struct _static_scope_t), NULL);
  scope->parent = self->static_scope;
  scope->binding = value;
  self->static_scope = scope;
}

void context_pop_static_scope(context_t self) {
  static_scope_t scope = self->static_scope;
  self->static_scope = scope->parent;
  allocator_free(self->allocator, scope);
}
static_scope_t context_get_static_scope(context_t self) {
  return self->static_scope;
}

char *const context_create_cstring(context_t self, const char *src) {
  char *str = create_cstring(self->allocator, src);
  array_push(self->strings, str);
  return str;
}

value_t context_get_undefined(context_t self) { return self->value_undefined; }

allocator_t context_get_allocator(context_t self) { return self->allocator; }
bool context_is_comptime(context_t ctx) { return ctx->comptime; }

bool context_set_comptime(context_t ctx, bool comptime) {
  bool current = ctx->comptime;
  ctx->comptime = comptime;
  return current;
}
void context_push_scope(context_t self) {
  self->scope = create_scope(self->allocator, self->scope);
}
void context_pop_scope(context_t self) {
  scope_t scope = self->scope;
  self->scope = scope_get_parent(self->scope);
  allocator_free(self->allocator, scope);
}
scope_t context_get_scope(context_t self) { return self->scope; }
void context_set_scope(context_t self, scope_t scope) { self->scope = scope; }