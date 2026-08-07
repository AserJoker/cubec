#ifndef _H_CUBEC_ENGINE_BOOL_TYPE_
#define _H_CUBEC_ENGINE_BOOL_TYPE_

#include "engine/stype.h"
#include "engine/context.h"
#include "engine/comptime_value.h"
#ifdef __cplusplus
extern "C" {
#endif

void bool_type_register(context_t ctx);
stype_t bool_type_get(context_t ctx);

/** @brief Create a bool comptime value. */
comptime_value_t bool_type_create_value(context_t ctx, bool val);

/** @brief Extract bool value. Returns false if not COMPTIME_VALUE_BOOL. */
bool bool_type_get_value(comptime_value_t val);

/** @brief Dispose a bool comptime value. */
void bool_type_dispose_value(comptime_value_t val);

/** @brief Clone a bool comptime value. */
comptime_value_t bool_type_clone_value(allocator_t allocator, comptime_value_t val);

/** @brief Compute structural hash of a bool comptime value. */
uint64_t bool_type_hash_value(comptime_value_t val);

#ifdef __cplusplus
}
#endif
#endif
