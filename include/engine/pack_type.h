#ifndef _H_CUBEC_ENGINE_PACK_TYPE_
#define _H_CUBEC_ENGINE_PACK_TYPE_
#include "engine/type.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Pack type — represents a variadic type parameter pack (...T).
 *
 * Compile-time only: used as the type of a rest generic parameter.
 * When a generic param is declared as `...T`, T's param_type is
 * TYPE_KIND_PACK rather than TYPE_KIND_TYPE.
 *
 * No runtime values exist for pack types; pack expansion happens
 * during generic instantiation (e.g., tuple<...T> expands the pack).
 */

/** @brief Get the "pack" type_t (static singleton, size=0). */
type_t type_get_pack_type(allocator_t allocator);

#ifdef __cplusplus
}
#endif
#endif
