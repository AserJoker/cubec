#ifndef _H_CUBEC_ENGINE_PTR_
#define _H_CUBEC_ENGINE_PTR_
#include "core/allocator.h"
#include "engine/type.h"
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_ptr_meta_t *cubec_ptr_meta_t;
struct _cubec_ptr_meta_t {
  cubec_type_t type;
  bool is_mutable;
  bool is_volatile;
};
cubec_ptr_meta_t cubec_create_ptr_meta(cubec_allocator_t allocator,
                                       cubec_type_t type, bool is_mutable,
                                       bool is_volatile);
#ifdef __cplusplus
}
#endif
#endif