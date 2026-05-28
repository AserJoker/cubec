#ifndef _H_ENGINE_CONTEXT_
#define _H_ENGINE_CONTEXT_
#include "core/allocator.h"
#include "core/array.h"
#include "core/hash_map.h"
#include "core/list.h"
#include "core/rbtree.h"
#include "core/stream.h"
#include "engine/module.h"
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
  module_t mod;
  type_t global;
  type_t self;
  scope_t root;
  scope_t current;
  context_type_t type;
  list_t trace;
  list_t dependences;
  value_t function;
  bool comptime;
  array_t functions;
};
typedef struct _trace_t *trace_t;
struct _trace_t {
  const char *filename;
  const char *funcname;
  size_t column;
  size_t line;
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
void context_push_scope(context_t ctx);
void context_pop_scope(context_t ctx);
void context_push_trace(context_t ctx, const char *filename,
                        const char *funcname, size_t column, size_t line);
void context_pop_trace(context_t ctx);
value_t context_load_module(context_t ctx, const char *name);
stream_t context_write_module(context_t ctx, const char *filename);
value_t context_declar(context_t ctx, const char *name, value_t value);
void context_push_error(context_t ctx, value_t err);
#ifdef __cplusplus
}
#endif
#endif