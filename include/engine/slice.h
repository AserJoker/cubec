#ifndef _H_ENGINE_SLICE_
#define _H_ENGINE_SLICE_
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif

type_t create_slice_type(context_t ctx, type_t base_type);
value_t create_comptime_slice(context_t ctx, type_t type, void *pdata,
                              size_t offset, size_t len, bool mutable);
type_t slice_type_get_type(type_t self);
size_t slice_get_len(value_t self);
size_t slice_get_offset(value_t self);
void *slice_get_data(value_t self);

#ifdef __cplusplus
}
#endif
#endif