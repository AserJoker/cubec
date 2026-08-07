#ifndef _H_CUBEC_ENGINE_FLOAT_TYPE_
#define _H_CUBEC_ENGINE_FLOAT_TYPE_

#include "engine/stype.h"
#include "engine/context.h"
#include "engine/comptime_value.h"
#ifdef __cplusplus
extern "C" {
#endif

void float_types_register(context_t ctx);
stype_t float_type_get(context_t ctx, enum type_kind_t kind);
bool type_kind_is_float(enum type_kind_t kind);

/** @brief Create a float comptime value. kind must be a float type_kind. */
comptime_value_t float_type_create_value(context_t ctx, enum type_kind_t kind, double val);

/** @brief Extract float value. Returns 0.0 if not COMPTIME_VALUE_FLOAT. */
double float_type_get_value(comptime_value_t val);

/** @brief Dispose a float comptime value. */
void float_type_dispose_value(comptime_value_t val);

/** @brief Clone a float comptime value. */
comptime_value_t float_type_clone_value(allocator_t allocator, comptime_value_t val);

/** @brief Compute structural hash of a float comptime value. */
uint64_t float_type_hash_value(comptime_value_t val);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_FLOAT_TYPE_ */
