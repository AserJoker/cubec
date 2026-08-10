#ifndef _H_CUBEC_ENGINE_WILDCARD_TYPE_
#define _H_CUBEC_ENGINE_WILDCARD_TYPE_
#include "engine/type.h"
#ifdef __cplusplus
extern "C" {
#endif

/** @brief Create the "wildcard" type_t (static singleton, no vtable).
 *  Placeholder in type computations: any type equal/extends wildcard is true. */
type_t type_create_wildcard_type(allocator_t allocator);

#ifdef __cplusplus
}
#endif
#endif
