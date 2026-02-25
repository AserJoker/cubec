#ifndef _H_CUBEC_ENGINE_SCOPE_
#define _H_CUBEC_ENGINE_SCOPE_
#include "core/allocator.h"
#include "core/list.h"
#include "core/map.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_scope_t *cubec_scope_t;
struct _cubec_scope_t {
  cubec_map_t types;
  cubec_map_t functions;
  cubec_map_t variables;
  cubec_scope_t parent;
  cubec_list_t children;
  cubec_list_t defers;
};
cubec_scope_t cubec_create_scope(cubec_allocator_t allocator,
                                 cubec_scope_t parent);
#ifdef __cplusplus
}
#endif
#endif