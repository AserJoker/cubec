#ifndef _H_CUBEC_ENGINE_COMPTIME_ALLOC_
#define _H_CUBEC_ENGINE_COMPTIME_ALLOC_
#include "core/allocator.h"
#include "core/strmap.h"
#include "engine/comptime_value.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Virtual memory allocator for comptime pointer semantics.
 *
 * Provides a safe address space where pointers are uint64_t keys instead of
 * real memory addresses.  Supports use-after-free detection and scope-based
 * lifetime management so that comptime variables going out of scope
 * automatically invalidate their addresses.
 */
struct comptime_alloc {
  allocator_t allocator;
  uint64_t next_addr;       /**< Next free address (starts at 1; 0 = null) */
  strmap_t allocations;     /**< "addr_str" -> comptime_value_t (owned) */
  strmap_t lifetimes;       /**< "addr_str" -> scope_depth (as void* encoded) */
  int scope_depth;          /**< Current scope nesting depth */
};

typedef struct comptime_alloc *comptime_allocator_t;

comptime_allocator_t comptime_allocator_create(allocator_t allocator);
void comptime_allocator_dispose(comptime_allocator_t self);

/**
 * @brief Allocate a virtual address for a value at the given scope depth.
 * @return The allocated virtual address (non-zero).
 */
uint64_t comptime_alloc_allocate(comptime_allocator_t self,
                                  comptime_value_t value, int scope_depth);

/**
 * @brief Read the value at a virtual address.
 * @return The value, or NULL if the address is freed/invalid (dangling).
 */
comptime_value_t comptime_alloc_read(comptime_allocator_t self, uint64_t addr);

/**
 * @brief Write a value to a virtual address.
 * @return true on success, false if address is freed/invalid.
 */
bool comptime_alloc_write(comptime_allocator_t self, uint64_t addr,
                           comptime_value_t value);

/** @brief Free a virtual address (marks it as dangling). */
void comptime_alloc_free(comptime_allocator_t self, uint64_t addr);

/** @brief Enter a new scope level. */
void comptime_alloc_enter_scope(comptime_allocator_t self);

/**
 * @brief Leave the current scope level.
 *        All allocations at depth >= current depth are freed.
 */
void comptime_alloc_leave_scope(comptime_allocator_t self);

#ifdef __cplusplus
}
#endif
#endif
