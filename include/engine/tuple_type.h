#ifndef _H_CUBEC_ENGINE_TUPLE_TYPE_
#define _H_CUBEC_ENGINE_TUPLE_TYPE_

#include "engine/stype.h"
#include "engine/context.h"
#include "engine/comptime_value.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get or create a tuple type from its element types.
 *
 * Tuple types are deduplicated by structural hash (element type hashes).
 * name is "(A,B,C)" where A,B,C are element type names.
 */
stype_t tuple_type_get_or_create(context_t ctx, vec_t element_types);

/** @brief Check if a type_kind_t is a tuple type. */
bool type_kind_is_tuple(enum type_kind_t kind);

/* ---- Tuple comptime value operations ---- */

void tuple_type_dispose_value(comptime_value_t val);
comptime_value_t tuple_type_clone_value(allocator_t allocator, comptime_value_t val);
uint64_t tuple_type_hash_value(comptime_value_t val);

#ifdef __cplusplus
}
#endif
#endif
