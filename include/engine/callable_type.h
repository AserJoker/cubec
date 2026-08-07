#ifndef _H_CUBEC_ENGINE_CALLABLE_TYPE_
#define _H_CUBEC_ENGINE_CALLABLE_TYPE_

#include "engine/stype.h"
#include "engine/context.h"
#include "engine/comptime_value.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get or create a callable (function type expression) type.
 *
 * Callable types are deduplicated by structural hash (param types + return type).
 * If not yet in ctx->types, creates and registers it.
 *
 * size=8, align=8 (function pointer on 64-bit).
 * name is "fn(A,B)R" where A,B are param type names and R is the return type name.
 * For void return, the format is "fn(A,B)".
 *
 * @param ctx          Compiler context
 * @param param_types  vec_t of stype_t (parameter types, may be NULL for no params)
 * @param return_type  Return type (may be void_type for no return value)
 */
stype_t callable_type_get_or_create(context_t ctx, vec_t param_types,
                                     stype_t return_type);

/** @brief Check if a type_kind_t is a callable type. */
bool type_kind_is_callable(enum type_kind_t kind);

/* ---- Callable comptime value operations ---- */

void callable_type_dispose_value(comptime_value_t val);
comptime_value_t callable_type_clone_value(allocator_t allocator, comptime_value_t val);
uint64_t callable_type_hash_value(comptime_value_t val);

#ifdef __cplusplus
}
#endif
#endif
