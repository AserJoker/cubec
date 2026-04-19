#ifndef _H_ENGINE_CONTEXT_
#define _H_ENGINE_CONTEXT_
#include "core/allocator.h"
#include "engine/scope.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _context_t *context_t;
context_t create_context(allocator_t allocator);
void context_push_scope(context_t self);
void context_pop_scope(context_t self);
scope_t context_get_scope(context_t self);
scope_t context_get_root_scope(context_t self);
const char *context_create_cstring(context_t self, const char *src);
allocator_t context_get_allocator(context_t self);
value_t context_load(context_t self, const char *name);
value_t context_create_value(context_t self, type_t type, void *data,
                             bool mutable, bool comptime, const char *name);
value_t context_load_module(context_t self, const char *filename);
#ifdef __cplusplus
}
#endif
#endif