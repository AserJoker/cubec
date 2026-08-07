#ifndef _H_CUBEC_ENGINE_FLOAT_TYPE_
#define _H_CUBEC_ENGINE_FLOAT_TYPE_

#include "engine/stype.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register float types (f16, f32, f64) into context's global scope.
 */
void float_types_register(context_t ctx);

/**
 * @brief Look up a float type by its type_kind_t.
 */
stype_t float_type_get(context_t ctx, enum type_kind_t kind);

/** @brief Check if a type_kind_t is a float type. */
bool type_kind_is_float(enum type_kind_t kind);

#ifdef __cplusplus
}
#endif
#endif /* _H_CUBEC_ENGINE_FLOAT_TYPE_ */
