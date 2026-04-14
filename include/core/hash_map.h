#ifndef _H_CUBEC_CORE_HASH_MAP_
#define _H_CUBEC_CORE_HASH_MAP_
#include "core/compare.h"
#include "core/list.h"
#include "hash.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_hash_map_t *cubec_hash_map_t;
typedef struct _cubec_hash_map_initialize_t {
  bool autofree_key;
  bool autofree_value;
  cubec_hash_fn_t hash;
  cubec_compare_fn_t compare;
} cubec_hash_map_initialize_t;

cubec_hash_map_t cubec_create_hash_map(cubec_allocator_t allocator,
                                       cubec_hash_map_initialize_t *initialize);
void cubec_hash_map_set(cubec_hash_map_t self, void *key, void *value,
                        void *hash_arg, void *cmp_arg);
void *cubec_hash_map_get(cubec_hash_map_t self, const void *key, void *hash_arg,
                         void *cmp_arg);
void cubec_hash_map_delete(cubec_hash_map_t self, const void *key,
                           void *hash_arg, void *cmp_arg);
bool cubec_hash_map_has(cubec_hash_map_t self, const void *key, void *hash_arg,
                        void *cmp_arg);
void *cubec_hash_map_move(cubec_hash_map_t map, const void *key, void *hash_arg,
                          void *cmp_arg);
void cubec_hash_map_clear(cubec_hash_map_t self);
size_t cubec_hash_map_get_size(cubec_hash_map_t self);
cubec_list_node_t cubec_hash_map_get_begin(cubec_hash_map_t self);
cubec_list_node_t cubec_hash_map_get_end(cubec_hash_map_t self);
cubec_list_node_t cubec_hash_map_get_first(cubec_hash_map_t self);
cubec_list_node_t cubec_hash_map_get_last(cubec_hash_map_t self);
cubec_list_node_t cubec_hash_map_node_get_next(cubec_list_node_t self);
cubec_list_node_t cubec_hash_map_node_get_last(cubec_list_node_t self);
void *cubec_hash_map_node_get_key(cubec_list_node_t self);
void *cubec_hash_map_node_get_value(cubec_list_node_t self);
void cubec_hash_map_node_set_key(cubec_list_node_t self, cubec_hash_map_t map,
                                 void *key);
void cubec_hash_map_node_set_value(cubec_list_node_t self, cubec_hash_map_t map,
                                   void *value);

#ifdef __cplusplus
}
#endif
#endif