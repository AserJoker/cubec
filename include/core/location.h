#ifndef _H_CUBEC_CORE_LOCATION_
#define _H_CUBEC_CORE_LOCATION_
#include "core/allocator.h"
#include "core/position.h"
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
typedef struct _location_t location_t;
struct _location_t {
  const char *filename;
  position_t begin;
  position_t end;
};
char *location_get(location_t *loc, allocator_t allocator);
bool location_is(location_t *loc, const char *str);
#ifdef __cplusplus
}
#endif
#endif