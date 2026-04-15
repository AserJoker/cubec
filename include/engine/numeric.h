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

void init_numeric_type(context_t ctx);
value_t create_i8(context_t ctx, i8_t value, bool mutable, const char *name);
value_t create_i16(context_t ctx, i16_t value, bool mutable, const char *name);
value_t create_i32(context_t ctx, i32_t value, bool mutable, const char *name);
value_t create_i64(context_t ctx, i64_t value, bool mutable, const char *name);
value_t create_u8(context_t ctx, u8_t value, bool mutable, const char *name);
value_t create_u16(context_t ctx, u16_t value, bool mutable, const char *name);
value_t create_u32(context_t ctx, u32_t value, bool mutable, const char *name);
value_t create_u64(context_t ctx, u64_t value, bool mutable, const char *name);
value_t create_f32(context_t ctx, f32_t value, bool mutable, const char *name);
value_t create_f64(context_t ctx, f64_t value, bool mutable, const char *name);
#ifdef __cplusplus
}
#endif
#endif