#ifndef _H_CUBEC_CORE_RBTREE_
#define _H_CUBEC_CORE_RBTREE_
#include "core/class.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Red-black tree keyed by uint64_t. Standard insert/find/remove with
 *        left/right rotation, insert fixup, and delete fixup.
 */
struct _rbtree_t;
typedef struct _rbtree_t *rbtree_t;

/** @brief Initialization parameters for rbtree_t. */
typedef struct _rbtree_init_t rbtree_init_t;
struct _rbtree_init_t {
  bool auto_dispose;  /**< If true, auto-free values on remove/clear/destroy */
};

/** @brief In-order iterator for rbtree_t. */
typedef struct _rbtree_iter_t rbtree_iter_t;
struct _rbtree_iter_t {
  rbtree_t tree;      /**< The tree being iterated */
  void *current;      /**< Current node pointer (internal) */
};

/** @brief Virtual table for rbtree_t. */
extern class_t g_rbtree_class;

/** @brief Get the number of entries. */
size_t rbtree_get_size(rbtree_t self);

/**
 * @brief Find the value for key.
 * @return The value pointer, or NULL if not found.
 */
void *rbtree_find(rbtree_t self, uint64_t key);

/**
 * @brief Insert or replace a key-value pair.
 * @return The previous value if key already existed, NULL if new insertion.
 */
void *rbtree_insert(rbtree_t self, uint64_t key, void *value);

/**
 * @brief Remove the entry for key.
 * @return Number of entries removed (0 or 1).
 */
size_t rbtree_remove(rbtree_t self, uint64_t key);

/** @brief Remove all entries. */
void rbtree_clear(rbtree_t self);

/** @brief Create an in-order iterator positioned before the first element. */
rbtree_iter_t rbtree_iter_first(rbtree_t tree);

/** @brief Advance the iterator and return the next value (NULL when done). */
void *rbtree_iter_next(rbtree_iter_t *iter);

#ifdef __cplusplus
}
#endif
#endif
