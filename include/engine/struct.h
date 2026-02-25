#ifndef _H_CUBEC_ENGINE_STRUCT_
#define _H_CUBEC_ENGINE_STRUCT_
#include "core/allocator.h"
#include "core/map.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_struct_type_t {
  struct _cubec_type_t super;
  cubec_map_t fields;
  cubec_map_t attributes;
  cubec_map_t methods;
} *cubec_struct_type_t;
typedef struct _cubec_struct_value_t {
  struct _cubec_value_t super;
  cubec_map_t fields;
} *cubec_struct_value_t;
cubec_type_t cubec_create_struct_type(cubec_allocator_t allocator,
                                      const char *name);
cubec_value_t cubec_create_struct_value(cubec_allocator_t allocator,
                                        cubec_type_t type);
#ifdef __cplusplus
}
#endif
#endif