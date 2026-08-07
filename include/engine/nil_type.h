#ifndef _H_CUBEC_ENGINE_NIL_TYPE_
#define _H_CUBEC_ENGINE_NIL_TYPE_

#include "engine/stype.h"
#include "engine/context.h"
#include "engine/comptime_value.h"
#ifdef __cplusplus
extern "C" {
#endif

void nil_type_register(context_t ctx);
stype_t nil_type_get(context_t ctx);

/** @brief Create a nil comptime value. */
comptime_value_t nil_type_create_value(context_t ctx);

/** @brief Dispose a nil comptime value. */
void nil_type_dispose_value(comptime_value_t val);

/** @brief Clone a nil comptime value. */
comptime_value_t nil_type_clone_value(allocator_t allocator, comptime_value_t val);

/** @brief Compute structural hash of a nil comptime value. */
uint64_t nil_type_hash_value(comptime_value_t val);

#ifdef __cplusplus
}
#endif
#endif
