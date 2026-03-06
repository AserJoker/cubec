#ifndef _H_CUBEC_ENGINE_UNION_
#define _H_CUBEC_ENGINE_UNION_
#include "core/allocator.h"
#include "core/array.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_union_meta_t *cubec_union_meta_t;
struct _cubec_union_meta_t {
  cubec_array_t types;
};
cubec_union_meta_t cubec_create_union_meta(cubec_allocator_t allocator,
                                           cubec_array_t types);
#ifdef __cplusplus
}
#endif
#endif