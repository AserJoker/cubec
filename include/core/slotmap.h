#ifndef _H_CUBEC_CORE_SLOTMAP_
#define _H_CUBEC_CORE_SLOTMAP_
#include "core/allocator.h"
#include "core/type.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Slot handle: 48-bit index + 16-bit version (little-endian bitfield).
 *
 * Layout (uint64_t):
 *   bits [63:48]  version  (16 bits, max 65535)
 *   bits [47:0]   index    (48 bits, max ~281 trillion)
 */
typedef uint64_t slot_id_t;

#define SLOT_INDEX_BITS  48
#define SLOT_VERSION_BITS 16
#define SLOT_INDEX_MASK   ((UINT64_C(1) << SLOT_INDEX_BITS) - 1)
#define SLOT_VERSION_MASK ((UINT64_C(1) << SLOT_VERSION_BITS) - 1)

static inline uint64_t slot_id_index(slot_id_t id) {
  return id & SLOT_INDEX_MASK;
}

static inline uint16_t slot_id_version(slot_id_t id) {
  return (uint16_t)(id >> SLOT_INDEX_BITS);
}

static inline slot_id_t slot_id_make(uint64_t index, uint16_t version) {
  return ((uint64_t)version << SLOT_INDEX_BITS) | (index & SLOT_INDEX_MASK);
}

/** @brief A single entry in the SlotMap. */
typedef struct slot_entry_t {
  uint16_t version;  /**< Current version (increments on remove) */
  void    *ptr;      /**< Stored pointer, NULL = slot is free */
} slot_entry_t;

/** @brief SlotMap — pointer container with stable handles. */
typedef struct slotmap_t {
  allocator_t allocator;
  slot_entry_t *entries;  /**< Dense entry array (allocator-owned) */
  uint64_t     capacity;  /**< Allocated entry count */
  uint64_t     count;     /**< Live (occupied) entry count */
  uint32_t    *free_list; /**< Reusable slot indices (allocator-owned) */
  uint64_t     free_count;/**< Number of entries in free_list */
} slotmap_t;

/** @brief Type descriptor for allocator_create. */
extern type_t g_slotmap_type;

/**
 * @brief Create a SlotMap via allocator.
 * @param allocator  Allocator to use
 * @return New slotmap_t pointer
 */
slotmap_t *slotmap_create(allocator_t allocator);

/**
 * @brief Dispose a SlotMap, freeing all internal storage.
 *        Does NOT free the pointers stored in entries.
 */
void slotmap_dispose(slotmap_t *self);

/**
 * @brief Insert a pointer and get a stable handle.
 * @param self  SlotMap
 * @param ptr   Pointer to store (may be NULL)
 * @return slot_id_t handle for later retrieval
 */
slot_id_t slotmap_insert(slotmap_t *self, void *ptr);

/**
 * @brief Look up a pointer by handle.
 * @return Stored pointer, or NULL if the handle is stale/invalid
 */
void *slotmap_get(slotmap_t *self, slot_id_t id);

/**
 * @brief Remove a slot by handle. The stored pointer is NOT freed.
 * @return true if the slot was live and removed, false if stale/invalid
 */
bool slotmap_remove(slotmap_t *self, slot_id_t id);

/**
 * @brief Compact the SlotMap by trimming trailing free slots.
 *
 * Walks from the end of the entry array and removes consecutive free slots.
 * Also removes those indices from the free_list. May shrink the internal
 * allocation if the new count is much smaller than capacity.
 *
 * @return Number of slots trimmed
 */
uint64_t slotmap_compact(slotmap_t *self);

#ifdef __cplusplus
}
#endif
#endif
