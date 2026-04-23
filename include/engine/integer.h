#ifndef _H_ENGINE_INTEGER_
#define _H_ENGINE_INTEGER_
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
void integer_init(context_t ctx);
value_t create_i8(context_t ctx, bool mut, const char *name);
value_t create_comptime_i8(context_t ctx, int8_t val, bool mut,
                           const char *name);
value_t create_i16(context_t ctx, bool mut, const char *name);
value_t create_comptime_i16(context_t ctx, int16_t val, bool mut,
                            const char *name);
value_t create_i32(context_t ctx, bool mut, const char *name);
value_t create_comptime_i32(context_t ctx, int32_t val, bool mut,
                            const char *name);
value_t create_i64(context_t ctx, bool mut, const char *name);
value_t create_comptime_i64(context_t ctx, int64_t val, bool mut,
                            const char *name);
int64_t integer_get_value(value_t self);
value_t create_comptime_integer(context_t ctx, type_t type, int64_t val);
value_t create_integer(context_t ctx, type_t type);
#ifdef __cplusplus
}
#endif
#endif