#ifndef _H_ENGINE_PTR_
#define _H_ENGINE_PTR_
#include "engine/context.h"
#include "engine/type.h"
#ifdef __cplusplus
extern "C" {
#endif
type_t create_ptr_type(context_t ctx, type_t type, bool mut, bool vol);
type_t create_parray_type(context_t ctx, type_t type, bool mut, bool vol);
type_t ptr_type_get_type(type_t type);
bool ptr_type_is_mut(type_t type);
bool ptr_type_is_vol(type_t type);
#ifdef __cplusplus
}
#endif
#endif