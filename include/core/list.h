#ifndef _H_CUBEC_CORE_LIST_
#define _H_CUBEC_CORE_LIST_
#include "core/type.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Doubly-linked list of `void*` pointers.
 *        Indexed access optimizes traversal: starts from nearer end.
 */
struct _list_t;
typedef struct _list_t *list_t;

/** @brief A node in the doubly-linked list. */
typedef struct _list_node_t list_node_t;
struct _list_node_t {
  list_node_t *prev;  /**< Previous node (NULL for head) */
  list_node_t *next;  /**< Next node (NULL for tail) */
  void *data;         /**< Stored pointer */
};

/** @brief Initialization parameters for list_t. */
typedef struct _list_init_t list_init_t;
struct _list_init_t {
  bool auto_dispose;  /**< If true, auto-free elements on remove/clear/destroy */
};

/** @brief Iterator for list_t. */
typedef struct _list_iter_t list_iter_t;
struct _list_iter_t {
  list_t list;        /**< The list being iterated */
  void *current;      /**< Current node pointer (internal) */
};

/** @brief Virtual table for list_t. */
extern type_t g_list_type;

/** @brief Get the number of elements. */
size_t list_get_size(list_t self);

/** @brief Copy all elements into a newly allocated `void*[]` array. Caller must free. */
void **list_get_data(list_t self);

/** @brief Get the first element. */
void *list_get_first(list_t self);

/** @brief Get the last element. */
void *list_get_last(list_t self);

/** @brief Append to the tail. Returns the new index. */
size_t list_push(list_t self, void *data);

/** @brief Remove and return the tail element. */
void *list_pop(list_t self);

/** @brief Prepend to the head. Returns 0. */
size_t list_unshift(list_t self, void *data);

/** @brief Remove and return the head element. */
void *list_shift(list_t self);

/** @brief Insert element at idx (O(n)). Returns idx. */
size_t list_insert(list_t self, size_t idx, void *data);

/** @brief Remove all elements. Returns 0. */
size_t list_clear(list_t self);

/** @brief Create an iterator positioned before the first element. */
list_iter_t list_iter_first(list_t list);

/** @brief Advance the iterator and return the next element (NULL when done). */
void *list_iter_next(list_iter_t *iter);

/**
 * @brief O(1): Get data at the current iterator position without advancing.
 * @param iter Iterator positioned at the desired element.
 * @return Element data, or NULL if the iterator is exhausted.
 */
void *list_iter_get(list_iter_t *iter);

/**
 * @brief O(1): Replace the element at the current iterator position.
 * @param iter Iterator positioned at the desired element.
 * @param data New data pointer to store.
 * @return The old data pointer (not auto-disposed, caller owns it).
 */
void *list_iter_set(list_iter_t *iter, void *data);

/**
 * @brief O(1): Remove the element at the current iterator position and
 *        advance the iterator to the next element.
 * @param iter Iterator positioned at the element to remove.
 * @return The removed element's data pointer (may be freed if auto_dispose
 *         is enabled), or NULL if the iterator is exhausted.
 */
void *list_iter_remove(list_iter_t *iter);

#ifdef __cplusplus
}
#endif
#endif