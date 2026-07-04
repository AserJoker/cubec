#ifndef _H_CUBEC_CORE_ALLOCATOR_
#define _H_CUBEC_CORE_ALLOCATOR_
#include "core/type.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>
/** @brief Opaque allocator handle. Tracks all allocations for leak detection. */
struct _allocator_t;
typedef struct _allocator_t *allocator_t;

/** @brief Custom allocation function signature (malloc-compatible). */
typedef void *(*alloc_fn_t)(size_t size);

/** @brief Custom deallocation function signature (free-compatible). */
typedef void (*free_fn_t)(void *);

/** @brief Dispose callback for allocator-managed objects. */
typedef void (*dispose_fn_t)(allocator_t allocator, void *self);
/**
 * @brief Create a new allocator. Calls abort() if the underlying alloc_fn fails.
 * @param alloc_fn Custom allocation function (NULL defaults to malloc)
 * @param free_fn Custom free function (NULL defaults to free)
 * @return A new allocator_t (never NULL; aborts on failure)
 */
allocator_t create_allocator(alloc_fn_t alloc_fn, free_fn_t free_fn);

/**
 * @brief Destroy the allocator and report all unfreed memory. NULL-safe.
 */
void delete_allocator(allocator_t allocator);

/**
 * @brief Allocate raw zero-initialized memory. Calls abort() on OOM.
 * @return Never NULL if size > 0; returns NULL only when size == 0.
 */
void *allocator_alloc(allocator_t self, size_t len);

/**
 * @brief Allocate and initialize a typed object. Calls abort() on OOM.
 * @return Never NULL (allocator_alloc aborts on failure).
 */
void *allocator_create(allocator_t self, type_t *type, void *arg);

/**
 * @brief Free allocated memory. NULL-safe. If the value has a type with
 *        dispose, the dispose function is called first.
 */
void allocator_free(allocator_t self, void *data);

/**
 * @brief Get the type descriptor of an allocated value.
 * @param self Pointer returned by allocator_alloc or allocator_create.
 * @return type_t* if the value was created via allocator_create, NULL if via allocator_alloc.
 */
type_t *value_get_type(void *self);

/**
 * @brief Get the unique allocation ID assigned to this value.
 * @param self Pointer returned by allocator_alloc or allocator_create.
 * @return Monotonically increasing ID within the allocator that created this value.
 */
uint64_t value_get_id(void *self);

/**
 * @brief Deep-clone an allocated value into a different allocator.
 *        The value must have a type with a registered clone function,
 *        otherwise an error is thrown.
 * @param allocator Target allocator for the clone.
 * @param another  The value to clone (may be NULL, returns NULL).
 * @return Cloned value in the target allocator, or NULL if another is NULL.
 */
void *value_clone(allocator_t allocator, void *another);

/**
 * @brief Move an allocated value from one allocator to another.
 *        The value must have a type with a registered move function,
 *        otherwise an error is thrown. The source value is invalidated.
 * @param allocator Target allocator for the moved value.
 * @param another   The value to move (may be NULL, returns NULL).
 * @return Moved value in the target allocator, or NULL if another is NULL.
 */
void *value_move(allocator_t allocator, void *another);
#ifdef __cplusplus
}
#endif
#endif