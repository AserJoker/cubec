#ifndef _H_C_VALUE_
#define _H_C_VALUE_
#include "core/stream.h"
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif
void write_c_value(context_t ctx, value_t value, stream_t stream);
#ifdef __cplusplus
}
#endif
#endif