#include "engine/context.h"
#include "ast/node.h"
#include "ast/node_type.h"
#include "core/allocator.h"
#include "core/compare.h"
#include "core/hash.h"
#include "core/hash_map.h"
#include "core/list.h"
#include "core/location.h"
#include "core/path.h"
#include "core/rbtree.h"
#include "core/string.h"
#include "engine/bool.h"
#include "engine/error.h"
#include "engine/integer.h"
#include "engine/module.h"
#include "engine/scope.h"
#include "engine/str.h"
#include "engine/struct.h"
#include "engine/type.h"
#include "engine/unsigned.h"
#include "engine/value.h"
#include "engine/void.h"
#include "resolve/program.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
static trace_t create_trace(allocator_t allocator, const char *filename,
                            size_t column, size_t line) {
  trace_t trace = allocator_alloc(allocator, sizeof(struct _trace_t), NULL);
  trace->filename = filename;
  trace->funcname = NULL;
  trace->column = column;
  trace->line = line;
  return trace;
}

static void context_dispose(context_t self, allocator_t allocator) {
  while (self->current) {
    context_pop_scope(self);
  }
  allocator_free(allocator, self->trace);
  allocator_free(allocator, self->strings);
  allocator_free(allocator, self->types);
  allocator_free(allocator, self->modules);
  allocator_free(allocator, self->dependences);
}
context_t create_context(allocator_t allocator) {
  context_t self = allocator_alloc(allocator, sizeof(struct _context_t),
                                   (dispose_fn_t)(context_dispose));
  self->allocator = allocator;
  rbtree_initialize_t strings_init = {
      .autofree = true,
      .compare = (compare_fn_t)strcmp,
  };
  self->strings = create_rbtree(allocator, &strings_init);
  hash_map_initialize_t types_init = {
      .hash = (hash_fn_t)cstring_sdb,
      .autofree_key = false,
      .autofree_value = true,
      .compare = (compare_fn_t)strcmp,
  };
  self->types = create_hash_map(allocator, &types_init);
  self->trace = create_list(allocator, &(list_initialize_t){true});
  list_append(self->trace, create_trace(self->allocator, NULL, 0, 0));
  hash_map_initialize_t modules_init = {
      .hash = (hash_fn_t)cstring_sdb,
      .autofree_key = false,
      .autofree_value = true,
      .compare = (compare_fn_t)strcmp,
  };
  self->modules = create_hash_map(allocator, &modules_init);
  self->dependences = create_list(allocator, &(list_initialize_t){
                                                 .autofree = true,
                                             });
  self->root = create_scope(allocator, NULL);
  self->current = self->root;
  self->global = NULL;
  self->self = NULL;
  self->mod = NULL;
  self->comptime = false;
  self->type = CONTEXT_TYPE_GLOBAL;
  init_type_type(self);
  init_void_type(self);
  init_error_type(self);
  init_bool_type(self);
  init_str_type(self);
  init_integer_type(self);
  init_unsigned_type(self);
  return self;
}
void context_store_type(context_t ctx, type_t type) {
  hash_map_set(ctx->types, (void *)type->id, type, NULL, NULL);
}
type_t context_load_type(context_t ctx, const char *id) {
  return hash_map_get(ctx->types, id, NULL, NULL);
}
const char *context_create_string(context_t ctx, const char *src) {
  if (!rbtree_has(ctx->strings, src, NULL)) {
    rbtree_put(ctx->strings, (void *)create_cstring(ctx->allocator, src), NULL);
  }
  return rbtree_get(ctx->strings, src, NULL);
}
value_t context_create_value(context_t ctx, type_t type, bool mut,
                             const char *name) {
  value_t value = create_value(ctx->allocator, type, mut);
  return context_declar(ctx, name, value);
}
value_t context_create_comptime_value(context_t ctx, type_t type, void *data,
                                      bool mut, const char *name) {
  value_t value = create_comptime_value(ctx->allocator, type, data, mut);
  return context_declar(ctx, name, value);
}
value_t context_create_weak_value(context_t ctx, type_t type, void *data,
                                  bool mut, const char *name) {
  value_t value = create_weak_value(ctx->allocator, type, data, mut);
  return context_declar(ctx, name, value);
}
value_t context_load(context_t ctx, const char *name) {
  scope_t scope = ctx->current;
  while (scope) {
    value_t value = scope_load(scope, name);
    if (value) {
      return value;
    }
    scope = scope->parent;
  }
  return NULL;
}

void context_push_scope(context_t ctx) {
  scope_t scope = create_scope(ctx->allocator, ctx->current);
  ctx->current = scope;
}

void context_pop_scope(context_t ctx) {
  scope_t scope = ctx->current->parent;
  allocator_free(ctx->allocator, ctx->current);
  ctx->current = scope;
}

void context_push_trace(context_t ctx, const char *filename,
                        const char *funcname, size_t column, size_t line) {
  trace_t trace = create_trace(ctx->allocator, filename, column, line);
  trace_t last = list_node_get(list_get_last(ctx->trace));
  last->funcname = funcname;
  list_append(ctx->trace, trace);
}

void context_pop_trace(context_t ctx) {
  list_erase(ctx->trace, list_get_last(ctx->trace));
}

static char *absolute(allocator_t allocator, const char *name) {
  path_t path = create_path(allocator, name);
  path_t fullpath = path_absolute(path, allocator);
  char *result = path_to_string(fullpath, allocator);
  allocator_free(allocator, path);
  allocator_free(allocator, fullpath);
  return result;
}

value_t context_load_module(context_t ctx, const char *filename) {
  char *fullname = absolute(ctx->allocator, filename);
  module_t mod = hash_map_get(ctx->modules, fullname, NULL, NULL);
  if (mod) {
    allocator_free(ctx->allocator, fullname);
    return mod->value;
  }
  ast_doc_t doc = read_ast_node(ctx->allocator, fullname, NULL);
  if (doc->node->type == NODE_TYPE_ERROR) {
    location_t loc = {
        .begin = doc->node->error->begin,
        .end = doc->node->error->end,
        .filename = doc->node->error->filename,
    };
    value_t err =
        create_comptime_error(ctx, loc, "%s", doc->node->error->message);
    err = value_clone(err, ctx->allocator);
    allocator_free(ctx->allocator, fullname);
    mod = create_module(ctx->allocator, filename, err, doc);
  } else {
    allocator_free(ctx->allocator, fullname);
    size_t module_count = hash_map_get_size(ctx->modules);
    size_t len = snprintf(NULL, 0, "M%" PRIuPTR, module_count);
    char id[len];
    sprintf(id, "M%" PRIuPTR, module_count);
    type_t stru = create_struct_type(ctx, id, "(nonamed)");
    value_t vstru = create_type_value(ctx, stru, false, NULL);
    vstru = value_clone(vstru, ctx->allocator);
    mod = create_module(ctx->allocator, filename, vstru, doc);
    if (ctx->mod == NULL) {
      mod->master = true;
    }
    context_type_t current_type = ctx->type;
    ctx->type = CONTEXT_TYPE_STRUCT;
    type_t current_global = ctx->global;
    ctx->global = stru;
    type_t current_self = ctx->self;
    ctx->self = stru;
    module_t current_module = ctx->mod;
    ctx->mod = mod;
    resolve_program(ctx, doc->node);
    ctx->mod = current_module;
    ctx->self = current_self;
    ctx->global = current_global;
    ctx->type = current_type;
    list_node_t it = list_get_first(mod->errors);
    while (it != list_get_end(mod->errors)) {
      value_t err = list_node_get(it);
      char *msg = error_format(ctx->allocator, err);
      fprintf(stderr, "%s\n", msg);
      allocator_free(ctx->allocator, msg);
      it = list_node_next(it);
    }
  }
  hash_map_set(ctx->modules, (void *)mod->filename, mod, NULL, NULL);
  value_t value = value_clone(mod->value, ctx->allocator);
  context_declar(ctx, NULL, value);
  return value;
}

value_t context_declar(context_t ctx, const char *name, value_t value) {
  if (name && scope_load(ctx->current, name)) {
    allocator_free(ctx->allocator, value);
    return create_error(ctx, "redefinition of '%s'", name);
  }
  scope_store(ctx->current, create_cstring(ctx->allocator, name), value);
  return value;
}
void context_push_error(context_t ctx, value_t err) {
  list_append(ctx->mod->errors, value_clone(err, ctx->allocator));
}