#ifndef _H_CUBEC_CORE_HASH_MAP_
#define _H_CUBEC_CORE_HASH_MAP_
#include "core/allocator.h"
#include "core/hash.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_hash_map_t *cubec_hash_map_t;
typedef struct _cubec_hash_map_node_t *cubec_hash_map_node_t;
typedef struct _cubec_hash_map_initialize_t {
  bool autofree_key;
  bool autofree_value;
  cubec_hash_fn_t hash;
} cubec_hash_map_initialize_t;
cubec_hash_map_t cubec_create_hash_map(cubec_allocator_t allocator,
                                       cubec_hash_map_initialize_t *initialize);
bool cubec_hash_map_set(cubec_hash_map_t self, cubec_allocator_t allocator,
                        const void *key, void *value, void *hash_arg);
void *cubec_hash_map_get(cubec_hash_map_t self, cubec_allocator_t allocator,
                         const void *key, void *hash_arg);
bool cubec_hash_map_has(cubec_hash_map_t self, cubec_allocator_t allocator,
                        const void *key, void *hash_arg);
bool cubec_hash_map_delete(cubec_hash_map_t self, cubec_allocator_t allocator,
                           const void *key, void *hash_arg);
bool cubec_hash_map_put(cubec_hash_map_t self, cubec_allocator_t allocator,
                        void *key, void *value, void *hash_arg);
void cubec_hash_map_clear(cubec_hash_map_t self, cubec_allocator_t allocator);
cubec_hash_map_node_t cubec_hash_map_get_begin(cubec_hash_map_t self);
cubec_hash_map_node_t cubec_hash_map_get_end(cubec_hash_map_t self);
cubec_hash_map_node_t cubec_hash_map_get_first(cubec_hash_map_t self);
cubec_hash_map_node_t cubec_hash_map_get_last(cubec_hash_map_t self);
cubec_hash_map_node_t cubec_hash_map_node_next(cubec_hash_map_node_t self,
                                               cubec_hash_map_t hmap);
cubec_hash_map_node_t cubec_hash_map_node_last(cubec_hash_map_node_t self,
                                               cubec_hash_map_t hmap);
void *cubec_hash_map_get_key(cubec_hash_map_node_t self);
void *cubec_hash_map_get_value(cubec_hash_map_node_t self);

#ifdef __cplusplus
}
#endif
#endif