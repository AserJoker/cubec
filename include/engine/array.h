#ifndef _H_CUBEC_ENGINE_ARRAY_
#define _H_CUBEC_ENGINE_ARRAY_
#include "core/allocator.h"
#include "core/array.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_array_type_t {
  struct _cubec_type_t super;
  cubec_type_t base_type;
  size_t length;
} *cubec_array_type_t;
typedef struct _cubec_array_value_t {
  struct _cubec_value_t super;
  cubec_array_t data;
} *cubec_array_value_t;
cubec_type_t cubec_create_array_type(cubec_allocator_t allocator,
                                     const char *name, cubec_type_t base_type,
                                     size_t length);
cubec_value_t cubec_create_array_value(cubec_allocator_t allocator,
                                       cubec_type_t type);
#ifdef __cplusplus
}
#endif
#endif