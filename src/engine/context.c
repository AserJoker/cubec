#include "engine/context.h"
#include "core/allocator.h"
#include "core/compare.h"
#include "core/hash.h"
#include "core/hash_map.h"
#include "core/list.h"
#include "core/rbtree.h"
#include "core/string.h"
#include "engine/bool.h"
#include "engine/error.h"
#include "engine/integer.h"
#include "engine/scope.h"
#include "engine/str.h"
#include "engine/unsigned.h"
#include "engine/value.h"
#include "engine/void.h"
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
  allocator_free(allocator, self->trace);
  allocator_free(allocator, self->strings);
  allocator_free(allocator, self->types);
  allocator_free(allocator, self->modules);
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
  context_push_trace(self, NULL, NULL, 0, 0);
  hash_map_initialize_t modules_init = {
      .hash = (hash_fn_t)cstring_sdb,
      .autofree_key = false,
      .autofree_value = true,
      .compare = (compare_fn_t)strcmp,
  };
  self->modules = create_hash_map(allocator, &modules_init);
  self->errors = create_list(allocator, &(list_initialize_t){
                                            .autofree = true,
                                        });
  self->dependences = create_list(allocator, &(list_initialize_t){
                                                 .autofree = true,
                                             });
  self->function = NULL;
  self->type = CONTEXT_TYPE_STRUCT;
  self->root = create_scope(allocator, NULL);
  self->current = self->root;
  self->global = NULL;
  self->self = NULL;
  self->mod = NULL;
  init_void_type(self);
  type_t void_t = context_load_type(self, "void");
  self->undefined =
      context_create_comptime_value(self, void_t, NULL, false, NULL);
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
  if (scope_load(ctx->current, name)) {
    return create_error(ctx, "redefinition of '%s'", name);
  }
  value_t value = create_value(ctx->allocator, type, mut);
  scope_store(ctx->current, create_cstring(ctx->allocator, name), value);
  return value;
}
value_t context_create_comptime_value(context_t ctx, type_t type, void *data,
                                      bool mut, const char *name) {
  if (scope_load(ctx->current, name)) {
    return create_error(ctx, "redefinition of '%s'", name);
  }
  value_t value = create_comptime_value(ctx->allocator, type, data, mut);
  scope_store(ctx->current, create_cstring(ctx->allocator, name), value);
  return value;
}
value_t context_create_weak_value(context_t ctx, type_t type, void *data,
                                  bool mut, const char *name) {

  if (scope_load(ctx->current, name)) {
    return create_error(ctx, "redefinition of '%s'", name);
  }
  value_t value = create_weak_value(ctx->allocator, type, data, mut);
  scope_store(ctx->current, create_cstring(ctx->allocator, name), value);
  return value;
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
value_t context_get_undefined(context_t ctx) { return ctx->undefined; }