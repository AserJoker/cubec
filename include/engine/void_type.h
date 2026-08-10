#ifndef _H_CUBEC_ENGINE_VOID_TYPE_
#define _H_CUBEC_ENGINE_VOID_TYPE_
#include "engine/type.h"
#ifdef __cplusplus
extern "C" {
#endif

/** @brief Get the "void" type_t (static singleton, size=0, align=0).
 *  Cannot create values. Supports type_equal/type_extends. */
type_t type_get_void_type(allocator_t allocator);

#ifdef __cplusplus
}
#endif
#endif
