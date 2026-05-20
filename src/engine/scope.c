#include "engine/scope.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/compare.h"
#include "core/hash.h"
#include "core/hash_map.h"
#include "core/list.h"
#include "core/string.h"
#include <string.h>
static void scope_dispose(scope_t scope, allocator_t allocator) {
  if (scope->parent) {
    list_t children = scope->parent->children;
    list_node_t it = list_get_first(children);
    while (it != list_get_end(children)) {
      if (list_node_get(it) == scope) {
        list_erase(children, it);
        break;
      }
      it = list_node_next(it);
    }
  }
  while (list_get_size(scope->children)) {
    list_node_t it = list_get_first(scope->children);
    scope_t child = list_node_get(it);
    allocator_free(allocator, child);
  }
  allocator_free(allocator, scope->children);
  allocator_free(allocator, scope->values);
  allocator_free(allocator, scope->variables);
}
scope_t create_scope(allocator_t allocator, scope_t parent) {
  scope_t self = allocator_alloc(allocator, sizeof(struct _scope_t),
                                 (dispose_fn_t)scope_dispose);
  self->parent = parent;
  self->children = create_list(allocator, NULL);
  if (parent) {
    list_append(parent->children, self);
  }
  self->values = create_array(allocator, &(array_initialize_t){
                                             .autofree = true,
                                         });
  self->variables =
      create_hash_map(allocator, &(hash_map_initialize_t){
                                     .hash = (hash_fn_t)cstring_sdb,
                                     .compare = (compare_fn_t)strcmp,
                                     .autofree_key = true,
                                     .autofree_value = false,
                                 });
  return self;
}
value_t scope_load(scope_t scope, const char *name) {
  return hash_map_get(scope->variables, name, NULL, NULL);
}
void scope_store(scope_t scope, char *name, value_t value) {
  if (name) {
    hash_map_set(scope->variables, name, value, NULL, NULL);
  }
  array_push(scope->values, value);
}