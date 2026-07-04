#ifndef _H_CUBEC_CORE_TYPE_
#define _H_CUBEC_CORE_TYPE_
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
struct _allocator_t;
/**
 * @brief Virtual table describing a type's lifecycle operations.
 *        Each "class" exports an `extern type_t g_xxx_type` instance.
 *        When an object is created via allocator_create(), the allocator
 *        records this type and automatically invokes init/dispose/clone/move.
 */
typedef struct _type_t type_t;

/** @brief Constructor: initialize self from arg (both opaque). */
typedef void (*type_init_fn_t)(void *self, struct _allocator_t *allocator,
                               void *arg);

/** @brief Destructor: release resources owned by self before the memory is freed. */
typedef void (*type_dispose_fn_t)(void *self, struct _allocator_t *allocator);

/** @brief Deep-copy another into self (both allocated in the given allocator). */
typedef void (*type_clone_fn_t)(void *self, struct _allocator_t *allocator,
                                void *another);

/** @brief Move ownership: transfer another's resources to self, invalidate another. */
typedef void (*type_move_fn_t)(void *self, struct _allocator_t *allocator,
                               void *another);

struct _type_t {
  const size_t size;              /**< Size of an instance in bytes */
  const char *name;               /**< Human-readable type name (for debugging) */
  type_init_fn_t init;            /**< Constructor (may be NULL) */
  type_dispose_fn_t dispose;      /**< Destructor (may be NULL) */
  type_clone_fn_t clone;          /**< Deep-copy (may be NULL) */
  type_move_fn_t move;            /**< Move (may be NULL) */
};
#ifdef __cplusplus
}
#endif
#endif