#ifndef _H_CUBEC_ENGINE_FLOAT_TYPE_
#define _H_CUBEC_ENGINE_FLOAT_TYPE_

#include "engine/stype.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

void float_types_register(context_t ctx);
stype_t float_type_get(context_t ctx, enum type_kind_t kind);
bool type_kind_is_float(enum type_kind_t kind);

/** @brief Hash a float value — interprets data as the float type (f16/f32/f64). */
uint64_t float_type_hash_value(stype_t type, uint64_t type_hash, const void *data);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_FLOAT_TYPE_ */
