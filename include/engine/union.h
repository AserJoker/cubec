#ifndef _H_CUBEC_ENGINE_UNION_
#define _H_CUBEC_ENGINE_UNION_
#include "core/allocator.h"
#include "core/array.h"
#include "engine/context.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
cubec_type_t cubec_context_union_type(cubec_context_t ctx, size_t align,
                                      const char *name);
void cubec_union_type_add_field(cubec_type_t self, cubec_allocator_t allocator,
                                const char *name, cubec_type_t type);
void cubec_union_type_add_attribute(cubec_type_t self,
                                    cubec_allocator_t allocator,
                                    const char *name, cubec_value_t value);
cubec_array_t cubec_union_type_get_fields(cubec_type_t self,
                                          cubec_allocator_t allocator);
cubec_array_t cubec_union_type_get_attributes(cubec_type_t self,
                                              cubec_allocator_t allocator);
cubec_type_t cubec_union_type_get_field(cubec_type_t self, const char *name);
cubec_value_t cubec_union_type_get_attribute(cubec_type_t self,
                                             const char *name);
#ifdef __cplusplus
}
#endif
#endif