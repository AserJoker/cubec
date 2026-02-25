#ifndef _H_CUBEC_ENGINE_REF_
#define _H_CUBEC_ENGINE_REF_
#include "core/allocator.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_ref_type_t {
  struct _cubec_type_t super;
  cubec_type_t base_type;
} *cubec_ref_type_t;
typedef struct _cubec_ref_value_t {
  struct _cubec_value_t super;
  cubec_value_t value;
} *cubec_ref_value_t;
cubec_type_t cubec_create_ref_type(cubec_allocator_t allocator,
                                   const char *name, cubec_type_t base_type);
cubec_value_t cubec_create_ref_value(cubec_allocator_t allocator,
                                     cubec_type_t type,
                                     cubec_value_t base_value);
cubec_value_t cubec_ref_value_get(cubec_value_t value);
#ifdef __cplusplus
}
#endif
#endif