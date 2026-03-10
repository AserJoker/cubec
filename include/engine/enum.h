#ifndef _H_CUBEC_ENGINE_ENUM_
#define _H_CUBEC_ENGINE_ENUM_
#include "core/allocator.h"
#include "core/map.h"
#include "engine/type.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_enum_meta_t *cubec_enum_meta_t;
struct _cubec_enum_meta_t {
  cubec_type_t type;
  cubec_map_t options;
};
cubec_enum_meta_t cubec_create_enum_meta(cubec_allocator_t allocator,
                                         cubec_type_t type);
#ifdef __cplusplus
}
#endif
#endif