#ifndef _H_ENGINE_UNSIGNED_
#define _H_ENGINE_UNSIGNED_
#include "engine/context.h"
#include "engine/value.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void unsigned_init(context_t ctx);
value_t create_u8(context_t ctx, bool mut, const char *name);
value_t create_comptime_u8(context_t ctx, uint8_t val, bool mut,
                           const char *name);
value_t create_u16(context_t ctx, bool mut, const char *name);
value_t create_comptime_u16(context_t ctx, uint16_t val, bool mut,
                            const char *name);
value_t create_u32(context_t ctx, bool mut, const char *name);
value_t create_comptime_u32(context_t ctx, uint32_t val, bool mut,
                            const char *name);
value_t create_u64(context_t ctx, bool mut, const char *name);
value_t create_comptime_u64(context_t ctx, uint64_t val, bool mut,
                            const char *name);
uint64_t unsigned_get_value(value_t self);
value_t create_comptime_unsigned(context_t ctx, type_t type, uint64_t val);
value_t create_unsigned(context_t ctx, type_t type);
#ifdef __cplusplus
}
#endif
#endif