#ifndef _H_CUBEC_ENGINE_STRUCT_
#define _H_CUBEC_ENGINE_STRUCT_
#include "core/allocator.h"
#include "core/array.h"
#include "core/map.h"
#include "engine/type.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct _cubec_struct_field_t *cubec_struct_field_t;
struct _cubec_struct_field_t {
  char *name;
  size_t offset;
  cubec_type_t type;
};
cubec_struct_field_t cubec_create_struct_field_desc(cubec_allocator_t allocator,
                                                    const char *name,
                                                    size_t offset,
                                                    cubec_type_t type);

cubec_array_t cubec_flat_struct_fields(cubec_allocator_t allocator,
                                       cubec_array_t fields);
typedef struct _cubec_struct_meta_t *cubec_struct_meta_t;
struct _cubec_struct_meta_t {
  cubec_array_t fields;
  cubec_map_t attributes;
  size_t align;
};
cubec_struct_meta_t cubec_create_struct_meta(cubec_allocator_t allocator);
#ifdef __cplusplus
}
#endif
#endif