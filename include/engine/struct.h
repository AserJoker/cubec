#ifndef _H_CUBEC_ENGINE_STRUCT_
#define _H_CUBEC_ENGINE_STRUCT_
#include "core/allocator.h"
#include "core/array.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_struct_meta_t *cubec_struct_meta_t;
cubec_struct_meta_t cubec_create_struct_meta(cubec_allocator_t allocator,
                                             size_t align, const char *name);
void cubec_struct_add_field(cubec_struct_meta_t self,
                            cubec_allocator_t allocator, const char *name,
                            cubec_type_t type);

void cubec_struct_add_attribute(cubec_struct_meta_t self,
                                cubec_allocator_t allocator, const char *name,
                                cubec_value_t value);
cubec_array_t cubec_struct_get_fields(cubec_struct_meta_t self,
                                      cubec_allocator_t allocator);
cubec_array_t cubec_struct_get_attributes(cubec_struct_meta_t self,
                                          cubec_allocator_t allocator);
cubec_type_t cubec_struct_get_field(cubec_struct_meta_t self, const char *name);
size_t cubec_struct_get_offset(cubec_struct_meta_t self, const char *name);
cubec_value_t cubec_struct_get_attribute(cubec_struct_meta_t self,
                                         const char *name);
#ifdef __cplusplus
}
#endif
#endif