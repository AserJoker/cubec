#ifndef _H_ENGINE_SCOPE_
#define _H_ENGINE_SCOPE_
#include "core/allocator.h"
#include "core/array.h"
#include "core/hash_map.h"
#include "core/list.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _scope_t *scope_t;
struct _scope_t {
  scope_t parent;
  list_t children;
  array_t values;
  hash_map_t variables;
};
scope_t create_scope(allocator_t allocator, scope_t parent);
value_t scope_load(scope_t scope, const char *name);
void scope_store(scope_t scope, char *name, value_t value);
#ifdef __cplusplus
}
#endif
#endif