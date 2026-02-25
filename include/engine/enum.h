#ifndef _H_CUBEC_ENGINE_ENUM_
#define _H_CUBEC_ENGINE_ENUM_
#include "core/allocator.h"
#include "core/map.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_enum_type_t {
  struct _cubec_type_t super;
  cubec_type_t base_type;
  cubec_map_t options;
} *cubec_enum_type_t;
typedef struct _cubec_enum_value_t {
  struct _cubec_value_t super;
  const char *option;
} *cubec_enum_value_t;
cubec_type_t cubec_create_enum_type(cubec_allocator_t allocator,
                                    const char *name, cubec_type_t base_type);
cubec_value_t cubec_create_enum_value(cubec_allocator_t allocator,
                                      cubec_type_t type, const char *option);
cubec_value_t cubec_enum_value_get(cubec_value_t value);
#ifdef __cplusplus
}
#endif
#endif