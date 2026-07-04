#ifndef _H_CUBEC_CORE_VEC_
#define _H_CUBEC_CORE_VEC_
#include "core/type.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/** @brief Virtual table for vec_t. */
extern type_t g_vec_type;

/**
 * @brief Dynamic array of `void*` pointers (like `std::vector<void*>`).
 *        Initial capacity is 0; first resize goes to 8, then doubles.
 */
struct _vec_t;
typedef struct _vec_t *vec_t;

/** @brief Initialization parameters for vec_t. */
typedef struct _vec_init_t vec_init_t;
struct _vec_init_t {
  bool auto_dispose;  /**< If true, auto-free elements on pop/remove/destroy */
};

/** @brief Get the number of elements. */
size_t vec_get_size(vec_t self);

/** @brief Get the current allocated capacity. */
size_t vec_get_capacity(vec_t self);

/** @brief Get a direct pointer to the internal `void*[]` buffer. */
void **vec_get_data(vec_t self);

/** @brief Get the element at index idx. */
void *vec_get(vec_t self, size_t idx);

/** @brief Set the element at index idx, returning the old value. */
size_t vec_set(vec_t self, size_t idx, void *data);

/** @brief Resize to exactly size elements. New slots are NULL. */
size_t vec_resize(vec_t self, size_t size);

/** @brief Append an element to the end. Returns the new index. */
size_t vec_push(vec_t self, void *data);

/** @brief Remove and return the last element. Returns removed index. */
size_t vec_pop(vec_t self);

/** @brief Remove element at idx, shifting subsequent elements left. Returns idx. */
size_t vec_remove(vec_t self, size_t idx);

/** @brief Insert element at idx, shifting existing elements right. Returns idx. */
size_t vec_insert(vec_t self, size_t idx, void *data);

/* ============================================================================
 *  Iterator
 * ============================================================================ */

/** @brief Iterator for vec_t. */
typedef struct _vec_iter_t vec_iter_t;
struct _vec_iter_t {
  vec_t vec;         /**< The vec being iterated */
  size_t idx;        /**< Current index */
};

/** @brief Create an iterator positioned at the first element (index 0). */
vec_iter_t vec_iter_first(vec_t vec);

/**
 * @brief Return the element at the current iterator position and advance.
 * @param iter Iterator positioned at the desired element.
 * @return Element data, or NULL if the iterator is exhausted.
 */
void *vec_iter_next(vec_iter_t *iter);

/**
 * @brief O(1): Get data at the current iterator position without advancing.
 * @param iter Iterator positioned at the desired element.
 * @return Element data, or NULL if the iterator is exhausted.
 */
void *vec_iter_get(vec_iter_t *iter);

/**
 * @brief O(1): Replace the element at the current iterator position.
 * @param iter Iterator positioned at the desired element.
 * @param data New data pointer to store.
 * @return The old data pointer (not auto-disposed, caller owns it).
 */
void *vec_iter_set(vec_iter_t *iter, void *data);

/**
 * @brief O(n): Remove the element at the current iterator position.
 *        Subsequent elements shift left; the iterator stays at the same
 *        index, now pointing to the next element.
 * @param iter Iterator positioned at the element to remove.
 * @return The removed element's data pointer (may be freed if auto_dispose
 *         is enabled), or NULL if the iterator is exhausted.
 */
void *vec_iter_remove(vec_iter_t *iter);
#ifdef __cplusplus
}
#endif
#endif