#ifndef _H_ENGINE_FLOAT_
#define _H_ENGINE_FLOAT_
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef _Float16 float16_t;
typedef float float32_t;
typedef double float64_t;

void init_float_type(context_t ctx);
value_t create_comptime_f16(context_t ctx, float16_t value, bool mut,
                            const char *name);
value_t create_comptime_f32(context_t ctx, float32_t value, bool mut,
                            const char *name);
value_t create_comptime_f64(context_t ctx, float64_t value, bool mut,
                            const char *name);
value_t create_f16(context_t ctx, bool mut, const char *name);
value_t create_f32(context_t ctx, bool mut, const char *name);
value_t create_f64(context_t ctx, bool mut, const char *name);
float64_t float_get_value(value_t value);
#ifdef __cplusplus
}
#endif
#endif