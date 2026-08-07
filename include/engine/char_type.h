#ifndef _H_CUBEC_ENGINE_CHAR_TYPE_
#define _H_CUBEC_ENGINE_CHAR_TYPE_

#include "engine/stype.h"
#include "engine/context.h"
#include "engine/comptime_value.h"
#ifdef __cplusplus
extern "C" {
#endif

void char_type_register(context_t ctx);
stype_t char_type_get(context_t ctx);

/** @brief Create a char comptime value (stored as uint64_t). */
comptime_value_t char_type_create_value(context_t ctx, uint64_t val);

/** @brief Extract char value. Returns 0 if not COMPTIME_VALUE_INT or wrong type. */
uint64_t char_type_get_value(comptime_value_t val);

#ifdef __cplusplus
}
#endif
#endif
