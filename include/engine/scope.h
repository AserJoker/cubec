#ifndef _H_CUBEC_ENGINE_SCOPE_
#define _H_CUBEC_ENGINE_SCOPE_
#include "core/allocator.h"
#include "engine/value.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_scope_t *cubec_scope_t;
cubec_scope_t cubec_create_scope(cubec_allocator_t allocator,
                                 cubec_scope_t parent);
void cubec_scope_store(cubec_scope_t self, cubec_allocator_t allocator,
                       cubec_value_t value, const char *name);
cubec_value_t cubec_scope_load(cubec_scope_t self, const char *name);
cubec_scope_t cubec_scope_get_parent(cubec_scope_t scope);
#ifdef __cplusplus
}
#endif
#endif