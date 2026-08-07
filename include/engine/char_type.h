#ifndef _H_CUBEC_ENGINE_CHAR_TYPE_
#define _H_CUBEC_ENGINE_CHAR_TYPE_

#include "engine/stype.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

void char_type_register(context_t ctx);
stype_t char_type_get(context_t ctx);

/** @brief Hash a char value — interprets data as uint8_t. */
uint64_t char_type_hash_value(stype_t type, uint64_t type_hash, const void *data);

#ifdef __cplusplus
}
#endif
#endif
