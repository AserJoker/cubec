#ifndef _H_CUBEC_CORE_MAP_
#define _H_CUBEC_CORE_MAP_
#include "core/class.h"
#include <stddef.h>
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Dictionary / map: stores `void*` key-value pairs.
 *        Small maps (< 16 entries) use a hash table (16-bucket chaining);
 *        large maps auto-convert to a red-black tree index.
 *        Keys are identified by alloc_get_id(key) (the allocator-assigned unique ID).
 */
struct _map_t;
typedef struct _map_t *map_t;

/** @brief Index backend type: hash table (< 16 entries) or red-black tree. */
typedef enum { MAP_INDEX_HASH, MAP_INDEX_RBTREE } map_index_type_t;

/** @brief Initialization parameters for map_t. */
typedef struct _map_init_t map_init_t;
struct _map_init_t {
  bool key_auto_dispose;    /**< If true, auto-free keys on remove/clear/destroy */
  bool value_auto_dispose;  /**< If true, auto-free values on remove/clear/destroy */
};

/** @brief Iterator for map_t. */
typedef struct _map_iter_t map_iter_t;
struct _map_iter_t {
  map_t map;              /**< The map being iterated */
  size_t current_idx;     /**< Current linear index (internal) */
};

/** @brief Virtual table for map_t. */
extern class_t g_map_class;

/** @brief Get the number of entries. */
size_t map_get_size(map_t self);

/**
 * @brief Find the value for a given key.
 * @param key The key to look up (matched by alloc_get_id).
 * @return The associated value, or NULL if not found.
 */
void *map_find(map_t self, void *key);

/**
 * @brief Insert a key-value pair. If key already exists, its value is replaced.
 * @return The new size of the map.
 */
size_t map_insert(map_t self, void *key, void *value);

/**
 * @brief Remove the entry for key.
 * @return The new size of the map.
 */
size_t map_remove(map_t self, void *key);

/** @brief Remove all entries. Returns 0. */
size_t map_clear(map_t self);

/** @brief Create an iterator positioned before the first entry. */
map_iter_t map_iter_first(map_t map);

/**
 * @brief Advance the iterator; returns the next key (or NULL when done).
 *        Use map_find(iter.map, key) to get the corresponding value.
 */
void *map_iter_next(map_iter_t *iter);

#ifdef __cplusplus
}
#endif
#endif