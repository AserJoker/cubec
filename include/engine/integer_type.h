#ifndef _H_CUBEC_ENGINE_INTEGER_TYPE_
#define _H_CUBEC_ENGINE_INTEGER_TYPE_

#include "engine/stype.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

void integer_types_register(context_t ctx);
stype_t integer_type_get(context_t ctx, enum type_kind_t kind);
bool type_kind_is_integer(enum type_kind_t kind);

/** @brief Hash an integer value — interprets data as the integer type (i8~u64). */
uint64_t integer_type_hash_value(stype_t type, uint64_t type_hash, const void *data);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_INTEGER_TYPE_ */
