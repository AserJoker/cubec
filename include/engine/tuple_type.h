#ifndef _H_CUBEC_ENGINE_TUPLE_TYPE_
#define _H_CUBEC_ENGINE_TUPLE_TYPE_

#include "engine/stype.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

stype_t tuple_type_get_or_create(context_t ctx, vec_t element_types);
bool type_kind_is_tuple(enum type_kind_t kind);

/** @brief Hash a tuple value — iterates elements at their offsets. */
uint64_t tuple_type_hash_value(context_t ctx, stype_t type, uint64_t type_hash, const void *data);

#ifdef __cplusplus
}
#endif
#endif
