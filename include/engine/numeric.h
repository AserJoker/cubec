#ifndef _H_CUBEC_ENGINE_NUMERIC_
#define _H_CUBEC_ENGINE_NUMERIC_
#include "engine/context.h"
#include "engine/value.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef _Float16 float16_t;
typedef float float32_t;
typedef double float64_t;
void cubec_init_numeric_type(cubec_context_t ctx);
cubec_value_t cubec_create_int8(cubec_context_t ctx, int8_t value, bool mutable,
                                const char *name);
cubec_value_t cubec_create_int16(cubec_context_t ctx, int16_t value,
                                 bool mutable, const char *name);
cubec_value_t cubec_create_int32(cubec_context_t ctx, int32_t value,
                                 bool mutable, const char *name);
cubec_value_t cubec_create_int64(cubec_context_t ctx, int64_t value,
                                 bool mutable, const char *name);
cubec_value_t cubec_create_uint8(cubec_context_t ctx, uint8_t value,
                                 bool mutable, const char *name);
cubec_value_t cubec_create_uint16(cubec_context_t ctx, uint16_t value,
                                  bool mutable, const char *name);
cubec_value_t cubec_create_uint32(cubec_context_t ctx, uint32_t value,
                                  bool mutable, const char *name);
cubec_value_t cubec_create_uint64(cubec_context_t ctx, uint64_t value,
                                  bool mutable, const char *name);
cubec_value_t cubec_create_float16(cubec_context_t ctx, float16_t value,
                                   bool mutable, const char *name);
cubec_value_t cubec_create_float32(cubec_context_t ctx, float32_t value,
                                   bool mutable, const char *name);
cubec_value_t cubec_create_float64(cubec_context_t ctx, float64_t value,
                                   bool mutable, const char *name);
#ifdef __cplusplus
}
#endif
#endif