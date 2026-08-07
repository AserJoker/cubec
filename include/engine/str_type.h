#ifndef _H_CUBEC_ENGINE_STR_TYPE_
#define _H_CUBEC_ENGINE_STR_TYPE_

#include "engine/stype.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

void str_type_register(context_t ctx);
stype_t str_type_get(context_t ctx);

/** @brief Hash a string value — looks up string_id in context string table. */
uint64_t str_type_hash_value(context_t ctx, stype_t type, uint64_t type_hash, const void *data);

#ifdef __cplusplus
}
#endif
#endif
