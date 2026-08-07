#ifndef _H_CUBEC_ENGINE_STR_TYPE_
#define _H_CUBEC_ENGINE_STR_TYPE_

#include "engine/stype.h"
#include "engine/context.h"
#include "engine/comptime_value.h"
#ifdef __cplusplus
extern "C" {
#endif

void str_type_register(context_t ctx);
stype_t str_type_get(context_t ctx);

/** @brief Create a str comptime value (takes ownership of string). */
comptime_value_t str_type_create_value(context_t ctx, string_t val);

/** @brief Create a str comptime value from C string (copied). */
comptime_value_t str_type_create_value_cstr(context_t ctx, const char *val);

/** @brief Extract string value (borrowing). Returns NULL if not COMPTIME_VALUE_STRING. */
string_t str_type_get_value(comptime_value_t val);

#ifdef __cplusplus
}
#endif
#endif
