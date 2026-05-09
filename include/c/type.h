#ifndef _H_C_TYPE_
#define _H_C_TYPE_
#include "core/stream.h"
#include "engine/context.h"
#include "engine/type.h"
#ifdef __cplusplus
extern "C" {
#endif
void write_c_type(context_t ctx, type_t type, stream_t stream);
#ifdef __cplusplus
}
#endif
#endif