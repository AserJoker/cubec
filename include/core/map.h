#ifndef _H_CUBEC_CORE_MAP_
#define _H_CUBEC_CORE_MAP_
#include "core/allocator.h"
#include "core/compare.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_map_t *cubec_map_t;
typedef struct _cubec_map_node_t *cubec_map_node_t;
typedef struct _cubec_map_initialize_t {
  bool autofree_key;
  bool autofree_value;
  cubec_compare_fn_t compare;
} cubec_map_initialize_t;
cubec_map_t cubec_create_map(cubec_allocator_t allocator,
                             cubec_map_initialize_t *initialize);
void cubec_map_set(cubec_map_t self, cubec_allocator_t allocator, void *key,
                   void *value, void *cmp_arg);
void *cubec_map_get(cubec_map_t self, void *key, void *cmp_arg);
void cubec_map_delete(cubec_map_t self, cubec_allocator_t allocator, void *key,
                      void *cmp_arg);
bool cubec_map_has(cubec_map_t self, void *key, void *cmp_arg);
bool cubec_map_clear(cubec_map_t self, cubec_allocator_t allocator);
size_t cubec_map_get_length(cubec_map_t self);
cubec_map_node_t cubec_map_get_begin(cubec_map_t self);
cubec_map_node_t cubec_map_get_end(cubec_map_t self);
cubec_map_node_t cubec_map_get_first(cubec_map_t self);
cubec_map_node_t cubec_map_get_last(cubec_map_t self);
void *cubec_map_node_get_key(cubec_map_node_t self);
void *cubec_map_node_get_value(cubec_map_node_t self);
void cubec_map_node_set_key(cubec_map_node_t self, cubec_allocator_t allocator,
                            cubec_map_t map, void *key);
void cubec_map_node_set_value(cubec_map_node_t self,
                              cubec_allocator_t allocator, cubec_map_t map,
                              void *value);
#ifdef __cplusplus
}
#endif
#endif