#include "engine/context.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "ast/program.h"
#include "c/program.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/compare.h"
#include "core/hash_map.h"
#include "core/location.h"
#include "core/position.h"
#include "core/rbtree.h"
#include "core/stream.h"
#include "core/string.h"
#include "engine/bool.h"
#include "engine/buitin.h"
#include "engine/error.h"
#include "engine/float.h"
#include "engine/function.h"
#include "engine/integer.h"
#include "engine/interrupt.h"
#include "engine/module.h"
#include "engine/null.h"
#include "engine/scope.h"
#include "engine/str.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/unsigned.h"
#include "engine/value.h"
#include "engine/void.h"
#include "fmt/program.h"
#include "resolve/function_declaration.h"
#include "resolve/program.h"
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
  value_t nil;
  value_t true_;
  value_t false_;
  context_type_t type;
  module_t module;
  value_t function;
  hash_map_t builtins;
  hash_map_t func_declars;
  array_t dependences;
};

static void context_dispose(context_t self, allocator_t allocator) {
  while (self->scope) {
    context_pop_scope(self);
  }
  allocator_free(allocator, self->modules);
  allocator_free(allocator, self->strings);
  allocator_free(allocator, self->types);
  allocator_free(allocator, self->func_declars);
  allocator_free(allocator, self->builtins);
  allocator_free(allocator, self->dependences);
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
  hash_map_initialize_t builtin_initialize = {
      .hash = (hash_fn_t)cstring_sdb,
      .compare = (compare_fn_t)strcmp,
      .autofree_key = true,
      .autofree_value = false,
  };
  self->builtins = create_hash_map(allocator, &builtin_initialize);
  hash_map_initialize_t types_initialize = modules_initialize;
  self->types = create_hash_map(allocator, &types_initialize);
  hash_map_initialize_t func_declar_initialize = {
      .autofree_key = false,
      .autofree_value = true,
      .hash = (hash_fn_t)cstring_sdb,
      .compare = (compare_fn_t)strcmp,
  };
  self->func_declars = create_hash_map(allocator, &func_declar_initialize);
  self->dependences = create_array(allocator, NULL);
  self->comptime = true;
  type_init(self);
  error_init(self);
  void_init(self);
  bool_init(self);
  str_init(self);
  integer_init(self);
  unsigned_init(self);
  float_init(self);
  interrupt_init(self);
  null_init(self);
  function_init(self);
  self->undefined = context_create_value(self, context_load_type(self, "void"),
                                         NULL, false, true, NULL);
  self->nil = context_create_value(self, context_load_type(self, "null"), NULL,
                                   false, true, "nil");
  self->true_ = create_comptime_bool(self, true, false, NULL);
  self->false_ = create_comptime_bool(self, false, false, NULL);
  self->comptime = false;
  self->global = NULL;
  self->module = NULL;
  self->function = NULL;
  self->self = NULL;
  context_set_builtin(self, "error", builtin_error);
  context_set_builtin(self, "typeof", builtin_typeof);
  context_set_builtin(self, "alignof", builtin_alignof);
  context_set_builtin(self, "sizeof", builtin_sizeof);
  context_set_builtin(self, "print", builtin_print);
  return self;
}
bool context_is_comptime(context_t ctx) { return ctx->comptime; }
bool context_set_comptime(context_t ctx, bool comptime) {
  bool current = ctx->comptime;
  ctx->comptime = comptime;
  return current;
}
type_t context_get_global(context_t ctx) { return ctx->global; }
type_t context_set_global(context_t ctx, type_t global) {
  type_t current = ctx->global;
  ctx->global = global;
  return current;
}

context_type_t context_get_type(context_t ctx) { return ctx->type; }
context_type_t context_set_type(context_t ctx, context_type_t type) {
  context_type_t current = ctx->type;
  ctx->type = type;
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
scope_t context_get_root_scope(context_t self) { return self->root; }
void context_set_root_scope(context_t self, scope_t scope) {
  self->root = scope;
}
void context_set_builtin(context_t ctx, const char *name, builtin_fn_t fn) {
  hash_map_set(ctx->builtins, create_cstring(ctx->allocator, name), fn, NULL,
               NULL);
}
ast_node_t context_eval_builtin(context_t ctx, const char *name, size_t argc,
                                ast_node_t *argv) {
  builtin_fn_t fn = hash_map_get(ctx->builtins, name, NULL, NULL);
  return fn(ctx, argc, argv);
}

bool context_has_builtin(context_t ctx, const char *name) {
  return hash_map_has(ctx->builtins, name, NULL, NULL);
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
  value_t value = context_load_local(self, name);
  if (!value) {
    value = context_load_global(self, name);
  }
  if (!value) {
    return create_error(self, "use of undeclared identifier '%s'", name);
  }
  return value;
}

value_t context_load_global(context_t self, const char *name) {
  struct_attribute_t attr = struct_type_get_attribute(self->global, name);
  if (attr) {
    return attr->value;
  }
  return NULL;
}

value_t context_load_local(context_t self, const char *name) {
  if (strcmp(name, "true") == 0) {
    return self->true_;
  }
  if (strcmp(name, "false") == 0) {
    return self->false_;
  }
  if (strcmp(name, "undefined") == 0) {
    return self->undefined;
  }
  if (strcmp(name, "__self__") == 0) {
    return create_type_value(self, self->self, false, NULL);
  }
  scope_t scope = self->scope;
  while (scope) {
    value_t value = scope_load(scope, name);
    if (value) {
      return value;
    }
    scope = scope_get_parent(scope);
  }
  return NULL;
}

value_t context_declar(context_t self, const char *name, value_t value) {
  scope_t scope = self->scope;
  if (scope_load(scope, name)) {
    return create_error(self, "redefinition of '%s'", name);
  }
  scope_store(self->scope, self->allocator, name, value);
  return self->undefined;
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
  for (size_t idx = 0; idx < array_get_size(self->dependences); idx++) {
    const char *frame = array_get(self->dependences, idx);
    if (strcmp(frame, filename) == 0) {
      string_t dep = create_string(self->allocator, NULL);
      for (size_t i = 0; i < array_get_size(self->dependences); i++) {
        const char *frame = array_get(self->dependences, i);
        string_concat(dep, self->allocator, "  ");
        string_concat(dep, self->allocator, frame);
        string_concat(dep, self->allocator, " -> \n");
      }
      string_concat(dep, self->allocator, "  ");
      string_concat(dep, self->allocator, filename);
      const char *dep_str = string_get(dep);
      size_t len = snprintf(NULL, 0, "cycle dependence: \n%s", dep_str);
      char buf[len];
      sprintf(buf, "cycle dependence: \n%s", dep_str);
      allocator_free(self->allocator, dep);
      return create_error(self, buf);
    }
  }
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
  sprintf(id, "M%" PRIuPTR, hash_map_get_size(self->modules));
  type_t module_struct = create_struct_type(self, id, 1);
  struct_type_seal(self, module_struct);
  value_t global = create_type_value(self, module_struct, false, NULL);
  context_type_t current_type = self->type;
  type_t current_global = self->global;
  type_t current_self = self->self;
  scope_t current_scope = self->scope;
  module_t current_module = self->module;
  value_t current_function = self->function;
  bool is_comptime = self->comptime;
  module = create_module(self->allocator, global, node, buf, filename);
  scope_t scope = create_scope(self->allocator, self->root);
  self->global = module_struct;
  self->self = module_struct;
  self->scope = scope;
  self->module = module;
  self->function = NULL;
  self->type = CONTEXT_TYPE_STRUCT;
  self->comptime = true;
  hash_map_set(self->modules, (void *)module_get_filename(module), module, NULL,
               NULL);
  array_push(self->dependences, (void *)filename);
  resolve_program(self, node);
  array_pop(self->dependences);
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
  self->comptime = is_comptime;
  self->function = current_function;
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
string_t context_fmt_module(context_t self, const char *module) {
  module_t m = hash_map_get(self->modules, module, NULL, NULL);
  if (!m) {
    return NULL;
  }
  ast_node_t node = module_get_node(m);
  stream_t stream = create_stream(self->allocator);
  fmt_program(self->allocator, node, stream);
  string_t str = stream_get_string(stream);
  allocator_free(self->allocator, stream);
  return str;
}

string_t context_write_module(context_t self, const char *module) {
  module_t m = hash_map_get(self->modules, module, NULL, NULL);
  if (!m) {
    return NULL;
  }
  ast_node_t node = module_get_node(m);
  stream_t stream = create_stream(self->allocator);
  context_type_t current_type = self->type;
  type_t current_global = self->global;
  type_t current_self = self->self;
  scope_t current_scope = self->scope;
  module_t current_module = self->module;
  scope_t scope = create_scope(self->allocator, self->root);
  value_t global = module_get_value(m);
  type_t module_struct = *(type_t *)value_get_data(global);
  self->global = module_struct;
  self->self = module_struct;
  self->scope = scope;
  self->module = m;
  self->function = NULL;
  self->type = CONTEXT_TYPE_STRUCT;
  write_c_program(self, node, stream);
  allocator_free(self->allocator, scope);
  self->function = NULL;
  self->scope = current_scope;
  self->type = current_type;
  self->self = current_self;
  self->global = current_global;
  self->module = current_module;
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
function_declar_t context_load_function_declar(context_t self, const char *id) {
  return hash_map_get(self->func_declars, id, NULL, NULL);
}

void context_store_function_declar(context_t self, function_declar_t declar) {
  hash_map_set(self->func_declars, (void *)declar->id, declar, NULL, NULL);
}

context_frame_t context_push(context_t ctx, value_t func, context_type_t type,
                             type_t global, type_t self) {
  context_frame_t frame;
  allocator_t allocator = context_get_allocator(ctx);
  frame.current_function = context_set_function(ctx, func);
  frame.current_global = context_set_global(ctx, global);
  frame.current_self = context_set_self(ctx, self);
  frame.current_type = context_set_type(ctx, type);
  return frame;
}

void context_pop(context_t ctx, context_frame_t frame) {
  context_set_self(ctx, frame.current_self);
  context_set_global(ctx, frame.current_global);
  context_set_function(ctx, frame.current_function);
  context_set_type(ctx, frame.current_type);
}