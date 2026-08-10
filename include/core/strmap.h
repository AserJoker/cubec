#ifndef _H_CUBEC_CORE_STRMAP_
#define _H_CUBEC_CORE_STRMAP_
#include "core/class.h"
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief String-keyed map built on top of rbtree + vec.
 *        Keys are null-terminated C strings, internally wrapped as string_t
 *        objects. Values must be allocator-managed objects.
 *        Small maps (< 16 entries) use a hash table (16-bucket chaining);
 *        large maps auto-convert to a red-black tree index.
 */
struct _strmap_t;
typedef struct _strmap_t *strmap_t;

/** @brief Index backend type: hash table (< 16 entries) or red-black tree. */
typedef enum { STRMAP_INDEX_HASH, STRMAP_INDEX_RBTREE } strmap_index_type_t;

/** @brief Initialization parameters for strmap_t. */
typedef struct _strmap_init_t strmap_init_t;
struct _strmap_init_t {
  bool value_auto_dispose; /**< If true, auto-free values on remove/clear/destroy */
};

/** @brief Iterator for strmap_t. */
typedef struct _strmap_iter_t strmap_iter_t;
struct _strmap_iter_t {
  strmap_t map;       /**< The map being iterated */
  size_t current_idx; /**< Current linear index (internal) */
};

/** @brief Virtual table for strmap_t. */
extern class_t g_strmap_class;

/** @brief Get the number of entries. */
size_t strmap_get_size(strmap_t self);

/**
 * @brief Find the value for a string key.
 * @return The value pointer, or NULL if not found.
 */
void *strmap_find(strmap_t self, const char *key);

/**
 * @brief Insert or replace a key-value pair.
 *        The key string is copied internally as a string_t object.
 * @return The previous value if key already existed, NULL if new insertion.
 */
void *strmap_insert(strmap_t self, const char *key, void *value);

/**
 * @brief Remove the entry for key.
 * @return The removed value, or NULL if key was not found.
 */
void *strmap_remove(strmap_t self, const char *key);

/** @brief Remove all entries. */
void strmap_clear(strmap_t self);

/** @brief Create an iterator positioned before the first entry. */
strmap_iter_t strmap_iter_first(strmap_t self);

/**
 * @brief Advance the iterator and return the next key (NULL when done).
 *        Use strmap_find(iter.map, key) to get the corresponding value.
 */
const char *strmap_iter_next(strmap_iter_t *iter);

#ifdef __cplusplus
}
#endif
#endif
