#ifndef _H_CUBEC_CORE_LOCATION_
#define _H_CUBEC_CORE_LOCATION_
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "core/allocator.h"
#include "core/position.h"
typedef struct _cubec_location_t {
  cubec_position_t begin;
  cubec_position_t end;
} cubec_location_t;
char *cubec_location_get(cubec_location_t self, cubec_allocator_t allocator);
char *cubec_location_get_line(cubec_location_t self, cubec_allocator_t allocator);
bool cubec_location_is(cubec_location_t self, const char *str);
#ifdef __cplusplus
}
#endif
#endif