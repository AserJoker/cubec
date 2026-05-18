#include "engine/context.h"
#include "core/allocator.h"
#include "core/compare.h"
#include "core/hash.h"
#include "core/hash_map.h"
#include "core/rbtree.h"
#include "core/string.h"
#include "engine/error.h"
#include "engine/integer.h"
#include "engine/scope.h"
#include "engine/str.h"
#include "engine/value.h"
#include <string.h>
static void context_dispose(context_t self, allocator_t allocator) {
  allocator_free(allocator, self->strings);
  allocator_free(allocator, self->types);
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
  self->function = NULL;
  self->type = CONTEXT_TYPE_STRUCT;
  self->root = create_scope(allocator, NULL);
  self->current = self->root;
  self->global = NULL;
  self->self = NULL;
  init_error_type(self);
  init_str_type(self);
  init_integer_type(self);
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