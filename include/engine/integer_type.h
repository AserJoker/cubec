#ifndef _H_CUBEC_ENGINE_INTEGER_TYPE_
#define _H_CUBEC_ENGINE_INTEGER_TYPE_

#include "engine/stype.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register integer types (i8~i64, u8~u64) into context's global scope.
 *
 * Creates stype_t for each integer type with name/size/align,
 * pushes to ctx->types, and inserts into ctx->global_scope->names.
 */
void integer_types_register(context_t ctx);

/**
 * @brief Look up an integer type by its type_kind_t.
 * @return The stype_t, or NULL if kind is not an integer type.
 */
stype_t integer_type_get(context_t ctx, enum type_kind_t kind);

/** @brief Check if a type_kind_t is an integer type. */
bool type_kind_is_integer(enum type_kind_t kind);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_INTEGER_TYPE_ */
