#ifndef _H_CUBEC_ENGINE_POINTER_TYPE_
#define _H_CUBEC_ENGINE_POINTER_TYPE_

#include "engine/stype.h"
#include "engine/context.h"
#include "engine/comptime_value.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get or create a pointer type pointing to element_type.
 *
 * Pointer types are deduplicated by structural hash: pointer(i32) always
 * returns the same stype_t. If not yet in ctx->types, creates and registers it.
 *
 * size=8, align=8 (64-bit pointer).
 * name is "*T" where T is the element type's name.
 */
stype_t pointer_type_get_or_create(context_t ctx, stype_t element_type);

/** @brief Check if a type_kind_t is a pointer type. */
bool type_kind_is_pointer(enum type_kind_t kind);

#ifdef __cplusplus
}
#endif
#endif
