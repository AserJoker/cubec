#ifndef _H_CUBEC_ENGINE_REF_
#define _H_CUBEC_ENGINE_REF_
#include "core/allocator.h"
#include "engine/type.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_ref_meta_t *cubec_ref_meta_t;
struct _cubec_ref_meta_t {
  cubec_type_t type;
};
cubec_ref_meta_t cubec_create_ref_meta(cubec_allocator_t allocator,
                                       cubec_type_t type);
#ifdef __cplusplus
}
#endif
#endif