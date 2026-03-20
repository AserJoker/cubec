#ifndef _H_CUBEC_ENGINE_STRUCT_
#define _H_CUBEC_ENGINE_STRUCT_
#include "core/allocator.h"
#include "core/array.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_struct_meta_t *cubec_struct_meta_t;
typedef struct _cubec_struct_field_t *cubec_struct_field_t;
struct _cubec_struct_field_t {
  char *name;
  cubec_type_t type;
  size_t offset;
};
typedef struct _cubec_struct_attribute_t *cubec_struct_attribute_t;
struct _cubec_struct_attribute_t {
  char *name;
  cubec_value_t value;
};
struct _cubec_struct_meta_t {
  cubec_array_t fields;
  cubec_array_t attributes;
  size_t align;
  char *name;
};
cubec_struct_meta_t cubec_create_struct_meta(cubec_allocator_t allocator,
                                             size_t align, const char *name);
void cubec_struct_add_field(cubec_type_t stru, cubec_allocator_t allocator,
                            const char *name, cubec_type_t type);
void cubec_struct_add_attribute(cubec_type_t stru, cubec_allocator_t allocator,
                                const char *name, cubec_value_t value);
#ifdef __cplusplus
}
#endif
#endif