#ifndef _H_CORE_LOCATION_
#define _H_CORE_LOCATION_
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "core/allocator.h"
#include "core/position.h"
typedef struct _location_t {
  position_t begin;
  position_t end;
  const char *filename;
} location_t;
char *location_get(location_t self, allocator_t allocator);
char *location_get_str(location_t self, allocator_t allocator);
char *location_get_line(location_t self, allocator_t allocator);
bool location_is(location_t self, const char *str);
#ifdef __cplusplus
}
#endif
#endif