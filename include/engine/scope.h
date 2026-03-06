#ifndef _H_CUBEC_ENGINE_SCOPE_
#define _H_CUBEC_ENGINE_SCOPE_
#include "core/allocator.h"
#include "core/list.h"
#include "core/map.h"
#include "engine/type.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_scope_t *cubec_scope_t;
struct _cubec_scope_t {
  cubec_scope_t parent;
  cubec_list_t children;
  cubec_list_t variables;
  cubec_list_t defers;
  cubec_map_t named_variables;
  cubec_map_t types;
};
cubec_scope_t cubec_create_scope(cubec_allocator_t allocator,
                                 cubec_scope_t parent);
void cubec_scope_store_type(cubec_scope_t self, cubec_allocator_t allocator,
                            const char *name, cubec_type_t type);
cubec_type_t cubec_scope_load_type(cubec_scope_t self, const char *name);
void cubec_scope_store_value(cubec_scope_t self, cubec_allocator_t allocator,
                             cubec_value_t value, const char *name);
cubec_value_t cubec_scope_load_value(cubec_scope_t self, const char *name);

#ifdef __cplusplus
}
#endif
#endif