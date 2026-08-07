#ifndef _H_CUBEC_ENGINE_ARRAY_TYPE_
#define _H_CUBEC_ENGINE_ARRAY_TYPE_

#include "engine/stype.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

stype_t array_type_get_or_create(context_t ctx, stype_t element_type, uint64_t length);
bool type_kind_is_array(enum type_kind_t kind);

/** @brief Hash an array value — iterates elements. */
uint64_t array_type_hash_value(context_t ctx, stype_t type, uint64_t type_hash, const void *data);

#ifdef __cplusplus
}
#endif
#endif
