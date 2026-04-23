#ifndef _H_ENGINE_FLOAT_
#define _H_ENGINE_FLOAT_
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
void float_init(context_t ctx);
value_t create_f16(context_t ctx, bool mut, const char *name);
value_t create_comptime_f16(context_t ctx, _Float16 val, bool mut,
                            const char *name);
value_t create_f32(context_t ctx, bool mut, const char *name);
value_t create_comptime_f32(context_t ctx, float val, bool mut,
                            const char *name);
value_t create_f64(context_t ctx, bool mut, const char *name);
value_t create_comptime_f64(context_t ctx, double val, bool mut,
                            const char *name);
double float_get_value(value_t self);
value_t create_comptime_float(context_t ctx, type_t type, double val);
value_t create_float(context_t ctx, type_t type);
#ifdef __cplusplus
}
#endif
#endif