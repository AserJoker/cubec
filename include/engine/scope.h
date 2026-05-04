#ifndef _H_ENGINE_SCOPE_
#define _H_ENGINE_SCOPE_
#include "core/allocator.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _scope_t *scope_t;
scope_t create_scope(allocator_t allocator, scope_t parent);
value_t scope_load(scope_t self, const char *name);
void scope_store(scope_t self, allocator_t allocator, const char *name,
                 value_t value);
scope_t scope_get_parent(scope_t self);
#ifdef __cplusplus
}
#endif
#endif