#ifndef _H_CUBEC_ENGINE_CALLABLE_TYPE_
#define _H_CUBEC_ENGINE_CALLABLE_TYPE_

#include "engine/stype.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

stype_t callable_type_get_or_create(context_t ctx, vec_t param_types, stype_t return_type);
bool type_kind_is_callable(enum type_kind_t kind);

/** @brief Hash a callable value — interprets data as function pointer. */
uint64_t callable_type_hash_value(stype_t type, uint64_t type_hash, const void *data);

#ifdef __cplusplus
}
#endif
#endif
