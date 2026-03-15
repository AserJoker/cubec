#ifndef _H_CUBEC_ENGINE_UNION_
#define _H_CUBEC_ENGINE_UNION_
#include "core/allocator.h"
#include "core/array.h"
#include "engine/type.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _cubec_union_meta_t *cubec_union_meta_t;
struct _cubec_union_meta_t {
  cubec_array_t types;
};
cubec_union_meta_t cubec_create_union_meta(cubec_allocator_t allocator,
                                           size_t size, cubec_type_t *types);
typedef struct _cubec_union_data_t *cubec_union_data_t;
struct _cubec_union_data_t {
  cubec_type_t type;
  uint8_t data[0];
};
#ifdef __cplusplus
}
#endif
#endif