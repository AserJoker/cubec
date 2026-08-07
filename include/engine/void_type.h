#ifndef _H_CUBEC_ENGINE_VOID_TYPE_
#define _H_CUBEC_ENGINE_VOID_TYPE_

#include "engine/stype.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

void void_type_register(context_t ctx);
stype_t void_type_get(context_t ctx);

/** @brief Hash a void value (no payload — just returns type_hash). */
uint64_t void_type_hash_value(stype_t type, uint64_t type_hash, const void *data);

#ifdef __cplusplus
}
#endif
#endif
