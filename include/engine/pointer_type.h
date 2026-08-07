#ifndef _H_CUBEC_ENGINE_POINTER_TYPE_
#define _H_CUBEC_ENGINE_POINTER_TYPE_

#include "engine/stype.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

stype_t pointer_type_get_or_create(context_t ctx, stype_t element_type);
bool type_kind_is_pointer(enum type_kind_t kind);

/** @brief Hash a pointer value — interprets data as void*. */
uint64_t pointer_type_hash_value(stype_t type, uint64_t type_hash, const void *data);

#ifdef __cplusplus
}
#endif
#endif
