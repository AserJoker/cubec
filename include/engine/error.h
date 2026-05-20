#ifndef _H_ENGINE_ERROR_
#define _H_ENGINE_ERROR_
#include "core/allocator.h"
#include "core/location.h"
#include "engine/context.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _error_t *error_t;
struct _error_t {
  const char *message;
  location_t loc;
};
void init_error_type(context_t ctx);
value_t create_error(context_t ctx, const char *fmt, ...);
value_t create_comptime_error(context_t ctx, location_t loc, const char *fmt,
                              ...);
value_t convert_comptime_error(context_t ctx, location_t loc, value_t error);
char *error_format(allocator_t allocator, value_t error);
#ifdef __cplusplus
}
#endif
#endif