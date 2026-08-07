#ifndef _H_CUBEC_ENGINE_BOOL_TYPE_
#define _H_CUBEC_ENGINE_BOOL_TYPE_

#include "engine/stype.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

void bool_type_register(context_t ctx);
stype_t bool_type_get(context_t ctx);

/** @brief Hash a bool value — interprets data as bool. */
uint64_t bool_type_hash_value(stype_t type, uint64_t type_hash, const void *data);

#ifdef __cplusplus
}
#endif
#endif
