#ifndef _H_CUBEC_CORE_MAP_
#define _H_CUBEC_CORE_MAP_
#include "core/type.h"
#include <stddef.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

struct _map_t;
typedef struct _map_t *map_t;

typedef enum { MAP_INDEX_HASH, MAP_INDEX_RBTREE } map_index_type_t;

typedef struct _map_init_t map_init_t;
struct _map_init_t {
  bool key_auto_dispose;
  bool value_auto_dispose;
};

typedef struct _map_iter_t map_iter_t;
struct _map_iter_t {
  map_t map;
  size_t current_idx;
};

extern type_t g_map_type;

size_t map_get_size(map_t self);
void *map_find(map_t self, void *key);
size_t map_insert(map_t self, void *key, void *value);
size_t map_remove(map_t self, void *key);
size_t map_clear(map_t self);
map_iter_t map_iter_first(map_t map);
void *map_iter_next(map_iter_t *iter);

#ifdef __cplusplus
}
#endif
#endif