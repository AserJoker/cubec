#include "engine/context.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/program.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/compare.h"
#include "core/hash_map.h"
#include "core/position.h"
#include "core/rbtree.h"
#include "core/stream.h"
#include "core/string.h"
#include "engine/bool.h"
#include "engine/error.h"
#include "engine/float.h"
#include "engine/integer.h"
#include "engine/interrupt.h"
#include "engine/module.h"
#include "engine/scope.h"
#include "engine/str.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/unsigned.h"
#include "engine/value.h"
#include "engine/void.h"
#include "resolve/function_declaration.h"
#include "resolve/program.h"
#include "writer/program.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

struct _context_t {
  rbtree_t strings;
  hash_map_t types;
  scope_t root;
  scope_t scope;
  allocator_t allocator;
  hash_map_t modules;
  type_t global;
  type_t self;
  bool comptime;
  value_t undefined;
  value_t true_;
  value_t false_;
  context_type_t type;
  module_t module;
  value_t function;
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
  integer_init(self);
  unsigned_init(self);
  float_init(self);
  interrupt_init(self);
  self->undefined = context_create_value(self, context_load_type(self, "void"),
                                         NULL, false, true, NULL);
  self->true_ = create_comptime_bool(self, true, false, NULL);
  self->false_ = create_comptime_bool(self, false, false, NULL);
  self->comptime = false;
  self->global = NULL;
  self->module = NULL;
  self->function = NULL;
  return self;
}
bool context_is_comptime(context_t ctx) { return ctx->comptime; }
bool context_set_comptime(context_t ctx, bool comptime) {
  bool current = ctx->comptime;
  ctx->comptime = comptime;
  return current;
}
type_t context_get_global(context_t ctx) { return ctx->global; }
void context_set_global(context_t ctx, type_t global) { ctx->global = global; }

context_type_t context_get_type(context_t ctx) { return ctx->type; }
void context_set_type(context_t ctx, context_type_t type) { ctx->type = type; }
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
scope_t context_get_root_scope(context_t self) { return self->root; }
void context_set_root_scope(context_t self, scope_t scope) {
  self->root = scope;
}

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
value_t context_get_undefined(context_t self) { return self->undefined; }
value_t context_get_true(context_t self) { return self->true_; }
value_t context_get_false(context_t self) { return self->false_; }
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
  char *buf = allocator_alloc(self->allocator, len + 1, NULL);
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
    value_t err = create_comptime_error(self, node, "%s", node->error);
    allocator_free(self->allocator, node);
    allocator_free(self->allocator, buf);
    return err;
  }
  len = strlen(node->loc.filename);
  char id[len + 16];
  sprintf(id, "module(%s)", node->loc.filename);
  type_t module_struct = create_struct_type(self, NULL, id, 1);
  value_t global = create_type_value(self, module_struct, false, true, NULL);
  bool current_type = self->type;
  type_t current_global = self->global;
  type_t current_self = self->self;
  scope_t current_scope = self->scope;
  module_t current_module = self->module;
  module = create_module(self->allocator, global, node, buf, filename);
  scope_t scope = create_scope(self->allocator, self->root);
  self->global = module_struct;
  self->self = module_struct;
  self->scope = scope;
  self->module = module;
  hash_map_set(self->modules, (void *)module_get_filename(module), module, NULL,
               NULL);
  resolve_program(self, node);
  array_t functions = module_get_functions(module);
  for (size_t idx = 0; idx < array_get_size(functions); idx++) {
    value_t func = array_get(functions, idx);
    resolve_function_declaration(self, func);
  }
  array_t errors = module_get_errors(self->module);
  if (array_get_size(errors)) {
    global =
        create_error(self, "failed to compile: %s, found %" PRIuPTR " errors",
                     filename, array_get_size(errors));
    for (size_t idx = 0; idx < array_get_size(errors); idx++) {
      value_t err = array_get(errors, idx);
      fprintf(stderr, "%s\n", error_get_message(err));
    }
  }
  allocator_free(self->allocator, scope);
  self->scope = current_scope;
  self->type = current_type;
  self->self = current_self;
  self->global = current_global;
  self->module = current_module;
  return global;
}
void context_push_error(context_t self, value_t error) {
  array_t errors = module_get_errors(self->module);
  array_push(errors, value_clone(error, self->allocator));
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
string_t context_write_module(context_t self, const char *module) {
  module_t m = hash_map_get(self->modules, module, NULL, NULL);
  if (!m) {
    return NULL;
  }
  ast_node_t node = module_get_node(m);
  stream_t stream = create_stream(self->allocator);
  write_program(self->allocator, node, stream);
  string_t str = stream_get_string(stream);
  allocator_free(self->allocator, stream);
  return str;
}
module_t context_get_module(context_t self) { return self->module; }
type_t context_get_self(context_t self) { return self->self; }
type_t context_set_self(context_t ctx, type_t self) {
  type_t current = ctx->self;
  ctx->self = self;
  return current;
}
value_t context_set_function(context_t ctx, value_t function) {
  value_t current = ctx->function;
  ctx->function = function;
  return current;
}
value_t context_get_function(context_t ctx) { return ctx->function; }