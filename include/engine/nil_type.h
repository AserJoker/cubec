#ifndef _H_CUBEC_ENGINE_NIL_TYPE_
#define _H_CUBEC_ENGINE_NIL_TYPE_

#include "engine/stype.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

void nil_type_register(context_t ctx);
stype_t nil_type_get(context_t ctx);

/** @brief Hash a nil value (typed null pointer). */
uint64_t nil_type_hash_value(stype_t type, uint64_t type_hash, const void *data);

#ifdef __cplusplus
}
#endif
#endif
