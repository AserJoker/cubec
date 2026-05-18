#ifndef _H_ENGINE_CONTEXT_
#define _H_ENGINE_CONTEXT_
#include "core/allocator.h"
#include "core/hash_map.h"
#include "core/rbtree.h"
#include "engine/scope.h"
#include "engine/type.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef enum _context_type_t {
  CONTEXT_TYPE_GLOBAL,
  CONTEXT_TYPE_STRUCT,
  CONTEXT_TYPE_FUNCTION,
} context_type_t;

typedef struct _context_t *context_t;
struct _context_t {
  allocator_t allocator;
  hash_map_t types;
  rbtree_t strings;
  hash_map_t modules;
  type_t global;
  type_t self;
  scope_t root;
  scope_t current;
  context_type_t type;
  value_t function;
};
context_t create_context(allocator_t allocator);
void context_store_type(context_t ctx, type_t type);
type_t context_load_type(context_t ctx, const char *id);
const char *context_create_string(context_t ctx, const char *src);
value_t context_create_value(context_t ctx, type_t type, bool mut,
                             const char *name);
value_t context_create_comptime_value(context_t ctx, type_t type, void *data,
                                      bool mut, const char *name);
value_t context_create_weak_value(context_t ctx, type_t type, void *data,
                                  bool mut, const char *name);
value_t context_load(context_t ctx, const char *name);
#ifdef __cplusplus
}
#endif
#endif