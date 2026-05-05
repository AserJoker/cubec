#ifndef _H_CORE_MAP_
#define _H_CORE_MAP_
#include "core/allocator.h"
#include "core/compare.h"
#include "core/list.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _map_t *map_t;
typedef struct _map_initialize_t {
  bool autofree_key;
  bool autofree_value;
  compare_fn_t compare;
} map_initialize_t;
map_t create_map(allocator_t allocator, map_initialize_t *initialize);
void map_set(map_t self, void *key, void *value, void *cmp_arg);
void *map_get(map_t self, const void *key, void *cmp_arg);
void map_delete(map_t self, const void *key, void *cmp_arg);
bool map_has(map_t self, const void *key, void *cmp_arg);
void map_clear(map_t self);
size_t map_get_size(map_t self);
list_node_t map_get_begin(map_t self);
list_node_t map_get_end(map_t self);
list_node_t map_get_first(map_t self);
list_node_t map_get_last(map_t self);
list_node_t map_node_get_next(list_node_t self);
list_node_t map_node_get_last(list_node_t self);
void *map_node_get_key(list_node_t self);
void *map_node_get_value(list_node_t self);
void map_node_set_key(list_node_t self, map_t map, void *key);
void map_node_set_value(list_node_t self, map_t map, void *value);
void *map_move(map_t map, const void *key, void *cmp_arg);
#ifdef __cplusplus
}
#endif
#endif