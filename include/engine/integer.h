#ifndef _H_ENGINE_INTEGER_
#define _H_ENGINE_INTEGER_
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif
void init_integer_type(context_t ctx);
value_t create_comptime_i8(context_t ctx, int8_t value, bool mut,
                           const char *name);
value_t create_comptime_i16(context_t ctx, int16_t value, bool mut,
                            const char *name);
value_t create_comptime_i32(context_t ctx, int32_t value, bool mut,
                            const char *name);
value_t create_comptime_i64(context_t ctx, int64_t value, bool mut,
                            const char *name);
value_t create_i8(context_t ctx, bool mut, const char *name);
value_t create_i16(context_t ctx, bool mut, const char *name);
value_t create_i32(context_t ctx, bool mut, const char *name);
value_t create_i64(context_t ctx, bool mut, const char *name);
int64_t integer_get_value(value_t value);
#ifdef __cplusplus
}
#endif
#endif