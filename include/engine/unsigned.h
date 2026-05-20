#ifndef _H_ENGINE_UNSIGNED_
#define _H_ENGINE_UNSIGNED_
#include "engine/context.h"
#ifdef __cplusplus
extern "C" {
#endif
void init_unsigned_type(context_t ctx);
value_t create_comptime_u8(context_t ctx, uint8_t value, bool mut,
                           const char *name);
value_t create_comptime_u16(context_t ctx, uint16_t value, bool mut,
                            const char *name);
value_t create_comptime_u32(context_t ctx, uint32_t value, bool mut,
                            const char *name);
value_t create_comptime_u64(context_t ctx, uint64_t value, bool mut,
                            const char *name);
value_t create_u8(context_t ctx, bool mut, const char *name);
value_t create_u16(context_t ctx, bool mut, const char *name);
value_t create_u32(context_t ctx, bool mut, const char *name);
value_t create_u64(context_t ctx, bool mut, const char *name);
uint64_t unsigned_get_value(value_t value);
#ifdef __cplusplus
}
#endif
#endif