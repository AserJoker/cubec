#ifndef _H_CUBEC_ENGINE_ARRAY_TYPE_
#define _H_CUBEC_ENGINE_ARRAY_TYPE_

#include "engine/stype.h"
#include "engine/context.h"
#include "engine/comptime_value.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get or create a fixed-size array type.
 *
 * Array types are deduplicated by structural hash (element type + length).
 * If not yet in ctx->types, creates and registers it.
 *
 * size = element_size * length, align = element_align.
 * name is "[N]T" where N is the length and T is the element type's name.
 */
stype_t array_type_get_or_create(context_t ctx, stype_t element_type,
                                  uint64_t length);

/** @brief Check if a type_kind_t is an array type. */
bool type_kind_is_array(enum type_kind_t kind);

#ifdef __cplusplus
}
#endif
#endif
