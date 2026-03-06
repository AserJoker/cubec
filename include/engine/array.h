#ifndef _H_CUBEC_ENGINE_ARRAY_
#define _H_CUBEC_ENGINE_ARRAY_
#include "core/allocator.h"
#include "engine/type.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_array_meta_t *cubec_array_meta_t;
struct _cubec_array_meta_t {
  cubec_type_t type;
  size_t length;
};
cubec_array_meta_t cubec_create_array_meta(cubec_allocator_t allocator,
                                           cubec_type_t type, size_t length);
#ifdef __cplusplus
}
#endif
#endif