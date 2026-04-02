#ifndef _H_CUBEC_ENGINE_STRUCT_
#define _H_CUBEC_ENGINE_STRUCT_
#include "core/allocator.h"
#include "core/array.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
cubec_type_t cubec_context_create_struct_type(cubec_context_t ctx, size_t align,
                                              const char *name);
void cubec_struct_type_add_field(cubec_type_t self, cubec_allocator_t allocator,
                                 const char *name, cubec_type_t type);
void cubec_struct_type_add_attribute(cubec_type_t self,
                                     cubec_allocator_t allocator,
                                     const char *name, cubec_value_t value);
cubec_array_t cubec_struct_type_get_fields(cubec_type_t self,
                                           cubec_allocator_t allocator);
cubec_array_t cubec_struct_type_get_attributes(cubec_type_t self,
                                               cubec_allocator_t allocator);
cubec_type_t cubec_struct_type_get_field(cubec_type_t self, const char *name);
size_t cubec_struct_type_get_offset(cubec_type_t self, const char *name);
cubec_value_t cubec_struct_type_get_attribute(cubec_type_t self,
                                              const char *name);
cubec_value_t cubec_create_struct_value(cubec_context_t self, cubec_type_t type,
                                        bool mutable, const char *name);
#ifdef __cplusplus
}
#endif
#endif