#ifndef _H_CUBEC_ENGINE_UNION_
#define _H_CUBEC_ENGINE_UNION_
#include "core/allocator.h"
#include "core/array.h"
#include "engine/value.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_union_type_t *cubec_union_type_t;
struct _cubec_union_type_t {
  struct _cubec_type_t super;
  cubec_array_t items;
};
typedef struct _cubec_union_value_t *cubec_union_value_t;
struct _cubec_union_value_t {
  struct _cubec_value_t super;
  cubec_value_t value;
};
cubec_type_t cubec_create_union_type(cubec_allocator_t allocator,
                                     const char *name, cubec_array_t items);
cubec_value_t cubec_create_union_value(cubec_allocator_t allocator,
                                       cubec_type_t type, cubec_value_t value);

#ifdef __cplusplus
}
#endif
#endif