#ifndef _H_CUBEC_ENGINE_INTEGER_TYPE_
#define _H_CUBEC_ENGINE_INTEGER_TYPE_

#include "engine/stype.h"
#include "engine/context.h"
#include "engine/comptime_value.h"
#ifdef __cplusplus
extern "C" {
#endif

void integer_types_register(context_t ctx);
stype_t integer_type_get(context_t ctx, enum type_kind_t kind);
bool type_kind_is_integer(enum type_kind_t kind);

/** @brief Create an integer comptime value. kind must be an integer type_kind. */
comptime_value_t integer_type_create_value(context_t ctx, enum type_kind_t kind, uint64_t val);

/** @brief Extract integer value. Returns 0 if not COMPTIME_VALUE_INT. */
uint64_t integer_type_get_value(comptime_value_t val);

/** @brief Dispose an integer comptime value. */
void integer_type_dispose_value(comptime_value_t val);

/** @brief Clone an integer comptime value. */
comptime_value_t integer_type_clone_value(allocator_t allocator, comptime_value_t val);

/** @brief Compute structural hash of an integer comptime value. */
uint64_t integer_type_hash_value(comptime_value_t val);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_INTEGER_TYPE_ */
