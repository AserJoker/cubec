#ifndef _H_CORE_HASH_MAP_
#define _H_CORE_HASH_MAP_
#include "core/compare.h"
#include "core/list.h"
#include "hash.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _hash_map_t *hash_map_t;
typedef struct _hash_map_initialize_t {
  bool autofree_key;
  bool autofree_value;
  hash_fn_t hash;
  compare_fn_t compare;
} hash_map_initialize_t;

hash_map_t create_hash_map(allocator_t allocator,
                           hash_map_initialize_t *initialize);
void hash_map_set(hash_map_t self, void *key, void *value, void *hash_arg,
                  void *cmp_arg);
void *hash_map_get(hash_map_t self, const void *key, void *hash_arg,
                   void *cmp_arg);
void hash_map_delete(hash_map_t self, const void *key, void *hash_arg,
                     void *cmp_arg);
bool hash_map_has(hash_map_t self, const void *key, void *hash_arg,
                  void *cmp_arg);
void *hash_map_move(hash_map_t map, const void *key, void *hash_arg,
                    void *cmp_arg);
void hash_map_clear(hash_map_t self);
size_t hash_map_get_size(hash_map_t self);
list_node_t hash_map_get_begin(hash_map_t self);
list_node_t hash_map_get_end(hash_map_t self);
list_node_t hash_map_get_first(hash_map_t self);
list_node_t hash_map_get_last(hash_map_t self);
list_node_t hash_map_node_get_next(list_node_t self);
list_node_t hash_map_node_get_last(list_node_t self);
void *hash_map_node_get_key(list_node_t self);
void *hash_map_node_get_value(list_node_t self);
void hash_map_node_set_key(list_node_t self, hash_map_t map, void *key);
void hash_map_node_set_value(list_node_t self, hash_map_t map, void *value);

#ifdef __cplusplus
}
#endif
#endif