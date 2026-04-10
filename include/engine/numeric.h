#ifndef _H_CUBEC_ENGINE_NUMERIC_
#define _H_CUBEC_ENGINE_NUMERIC_
#include "engine/context.h"
#include "engine/value.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef _Float16 f16_t;
typedef float f32_t;
typedef double f64_t;
typedef int8_t i8_t;
typedef int16_t i16_t;
typedef int32_t i32_t;
typedef int64_t i64_t;
typedef uint8_t u8_t;
typedef uint16_t u16_t;
typedef uint32_t u32_t;
typedef uint64_t u64_t;

void cubec_init_numeric_type(cubec_context_t ctx);
cubec_value_t cubec_create_i8(cubec_context_t ctx, i8_t value, bool mutable,
                              const char *name);
cubec_value_t cubec_create_i16(cubec_context_t ctx, i16_t value, bool mutable,
                               const char *name);
cubec_value_t cubec_create_i32(cubec_context_t ctx, i32_t value, bool mutable,
                               const char *name);
cubec_value_t cubec_create_i64(cubec_context_t ctx, i64_t value, bool mutable,
                               const char *name);
cubec_value_t cubec_create_u8(cubec_context_t ctx, u8_t value, bool mutable,
                              const char *name);
cubec_value_t cubec_create_u16(cubec_context_t ctx, u16_t value, bool mutable,
                               const char *name);
cubec_value_t cubec_create_u32(cubec_context_t ctx, u32_t value, bool mutable,
                               const char *name);
cubec_value_t cubec_create_u64(cubec_context_t ctx, u64_t value, bool mutable,
                               const char *name);
cubec_value_t cubec_create_f32(cubec_context_t ctx, f32_t value, bool mutable,
                               const char *name);
cubec_value_t cubec_create_f64(cubec_context_t ctx, f64_t value, bool mutable,
                               const char *name);
#ifdef __cplusplus
}
#endif
#endif