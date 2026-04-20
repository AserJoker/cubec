#ifndef _H_ENGINE_REF_
#define _H_ENGINE_REF_
#include "engine/context.h"
#include "engine/type.h"
#ifdef __cplusplus
extern "C" {
#endif
type_t create_ref_type(context_t ctx, type_t type);
type_t ref_type_get_type(type_t self);
value_t create_ref_value(context_t ctx, value_t value);
value_t ref_get_value(context_t ctx, value_t self);
#ifdef __cplusplus
}
#endif
#endif