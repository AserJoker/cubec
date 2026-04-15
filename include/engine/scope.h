#ifndef _H_CUBEC_ENGINE_SCOPE_
#define _H_CUBEC_ENGINE_SCOPE_
#include "core/allocator.h"
#include "engine/value.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _scope_t *scope_t;
scope_t create_scope(allocator_t allocator, scope_t parent);
void scope_store(scope_t self, allocator_t allocator, value_t value,
                 const char *name);
value_t scope_load(scope_t self, const char *name);
scope_t scope_get_parent(scope_t scope);
#ifdef __cplusplus
}
#endif
#endif