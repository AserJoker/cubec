#include "engine/context.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/program.h"
#include "core/allocator.h"
#include "core/compare.h"
#include "core/hash_map.h"
#include "core/position.h"
#include "core/rbtree.h"
#include "core/string.h"
#include "engine/bool.h"
#include "engine/error.h"
#include "engine/module.h"
#include "engine/scope.h"
#include "engine/str.h"
#include "engine/type.h"
#include "engine/value.h"
#include "engine/void.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

struct _context_t {
  rbtree_t strings;
  hash_map_t types;
  scope_t root;
  scope_t scope;
  allocator_t allocator;
  hash_map_t modules;
};
static void context_dispose(context_t self, allocator_t allocator) {
  while (self->scope) {
    context_pop_scope(self);
  }
  allocator_free(allocator, self->modules);
  allocator_free(allocator, self->strings);
  allocator_free(allocator, self->types);
}
context_t create_context(allocator_t allocator) {
  context_t self = allocator_alloc(allocator, sizeof(struct _context_t),
                                   (dispose_fn_t)context_dispose);
  self->root = create_scope(allocator, NULL);
  self->scope = self->root;
  self->allocator = allocator;
  rbtree_initialize_t strings_initialize = {
      .autofree = true,
      .compare = (compare_fn_t)strcmp,
  };
  self->strings = create_rbtree(allocator, &strings_initialize);
  hash_map_initialize_t modules_initialize = {
      .autofree_key = false,
      .autofree_value = true,
      .hash = (hash_fn_t)cstring_sdb,
      .compare = (compare_fn_t)strcmp,
  };
  self->modules = create_hash_map(allocator, &modules_initialize);
  hash_map_initialize_t types_initialize = modules_initialize;
  self->types = create_hash_map(allocator, &types_initialize);
  type_init(self);
  error_init(self);
  void_init(self);
  bool_init(self);
  str_init(self);
  return self;
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
scope_t context_get_root_scope(context_t self) { return self->root; }

const char *context_create_cstring(context_t self, const char *src) {
  const char *current = rbtree_get(self->strings, src, NULL);
  if (!current) {
    char *str = create_cstring(self->allocator, src);
    rbtree_put(self->strings, str, NULL);
    return str;
  }
  return current;
}
allocator_t context_get_allocator(context_t self) { return self->allocator; }
value_t context_load(context_t self, const char *name) {
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
value_t context_create_value(context_t self, type_t type, const void *data,
                             bool mut, bool comptime, const char *name) {
  scope_t scope = context_get_scope(self);
  if (name && scope_load(scope, name)) {
    return create_error(self, "redefinition of '%s'", name);
  }
  allocator_t allocator = context_get_allocator(self);
  value_t value = create_value(allocator, type, mut, data, comptime);
  scope_store(scope, allocator, name, value);
  return value;
}
value_t context_create_weak_value(context_t self, type_t type, void *data,
                                  bool mut, const char *name) {
  scope_t scope = context_get_scope(self);
  if (name && scope_load(scope, name)) {
    return create_error(self, "redefinition of '%s'", name);
  }
  allocator_t allocator = context_get_allocator(self);
  value_t value = create_weak_value(allocator, type, mut, data);
  scope_store(scope, allocator, name, value);
  return value;
}
value_t context_load_module(context_t self, const char *filename) {
  module_t module = hash_map_get(self->modules, filename, NULL, NULL);
  if (module) {
    return module_get_value(module);
  }
  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    return create_error(self, "failed to open file %s", filename);
  }
  (void)fseek(fp, 0, SEEK_END);
  size_t len = ftell(fp);
  (void)fseek(fp, 0, SEEK_SET);
  char buf[len + 1];
  (void)fread(buf, len, 1, fp);
  (void)fclose(fp);
  buf[len] = 0;
  position_t pos = {
      .column = 0,
      .line = 0,
      .offset = buf,
  };
  ast_node_t node =
      read_ast_program(self->allocator, &pos, buf + len, filename);
  if (node->type == NODE_TYPE_ERROR) {
    ast_error_t error = (ast_error_t)node;
    value_t err = create_compile_error(self, node, "%s", error->message);
    allocator_free(self->allocator, node);
    return err;
  }
  value_t ctx = context_load(self, "undefined");
  module = create_module(self->allocator, ctx, node, filename);
  hash_map_set(self->modules, (void *)module_get_filename(module), module, NULL,
               NULL);
  return ctx;
}
void context_store_type(context_t self, type_t type) {
  const char *id = type_get_id(type);
  hash_map_set(self->types, (void *)id, type, NULL, NULL);
}
type_t context_load_type(context_t self, const char *id) {
  return hash_map_get(self->types, id, NULL, NULL);
}
value_t context_clone_value(context_t self, value_t value) {
  value_t val = value_clone(value, self->allocator);
  scope_store(self->scope, self->allocator, NULL, val);
  return val;
}