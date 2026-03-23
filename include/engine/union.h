#ifndef _H_CUBEC_ENGINE_UNION_
#define _H_CUBEC_ENGINE_UNION_
#include "core/allocator.h"
#include "core/array.h"
#include "engine/type.h"
#include "engine/value.h"
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct _cubec_union_meta_t *cubec_union_meta_t;
typedef struct _cubec_union_field_t *cubec_union_field_t;
struct _cubec_union_field_t {
  char *name;
  cubec_type_t type;
};
typedef struct _cubec_union_attribute_t *cubec_union_attribute_t;
struct _cubec_union_attribute_t {
  char *name;
  cubec_value_t value;
};
struct _cubec_union_meta_t {
  cubec_array_t fields;
  cubec_array_t attributes;
  size_t align;
  char *name;
};
cubec_union_meta_t cubec_create_union_meta(cubec_allocator_t allocator,
                                           size_t align, const char *name);
void cubec_union_add_field(cubec_type_t stru, cubec_allocator_t allocator,
                           const char *name, cubec_type_t type);
void cubec_union_add_attribute(cubec_type_t stru, cubec_allocator_t allocator,
                               const char *name, cubec_value_t value);
#ifdef __cplusplus
}
#endif
#endif