#ifndef _H_CUBEC_ENGINE_SLICE_TYPE_
#define _H_CUBEC_ENGINE_SLICE_TYPE_

#include "engine/stype.h"
#include "engine/context.h"
#include "engine/comptime_value.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get or create a slice type over element_type.
 *
 * Slice types are deduplicated by structural hash: slice(i32) always
 * returns the same stype_t. If not yet in ctx->types, creates and registers it.
 *
 * size=24, align=8 (ptr + start + length on 64-bit).
 * name is "[]T" where T is the element type's name.
 */
stype_t slice_type_get_or_create(context_t ctx, stype_t element_type);

/** @brief Check if a type_kind_t is a slice type. */
bool type_kind_is_slice(enum type_kind_t kind);

/* ---- Slice comptime value operations ---- */

void slice_type_dispose_value(comptime_value_t val);
comptime_value_t slice_type_clone_value(allocator_t allocator, comptime_value_t val);
uint64_t slice_type_hash_value(comptime_value_t val);

#ifdef __cplusplus
}
#endif
#endif
