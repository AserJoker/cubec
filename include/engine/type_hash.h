#ifndef _H_CUBEC_ENGINE_TYPE_HASH_
#define _H_CUBEC_ENGINE_TYPE_HASH_
#include "engine/semantic_type.h"
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compute a structural hash for a semantic type.
 *        The hash is based on the type's structure, not its name.
 *        Used for type deduplication and fast equality checks.
 */
size_t type_hash_compute(semantic_type_t type);

/**
 * @brief Ensure a type's impl has a computed hash.
 *        If the hash is already computed (non-zero), this is a no-op.
 *        Otherwise, computes and stores the hash.
 */
void type_hash_ensure(semantic_type_t type);

#ifdef __cplusplus
}
#endif
#endif
