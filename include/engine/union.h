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
struct _union_attribute_t {
  char *name;
  value_t value;
};
typedef struct _union_attribute_t *union_attribute_t;
struct _union_field_t {
  char *name;
  type_t type;
};
typedef struct _union_field_t *union_field_t;
value_t create_union_type(context_t ctx, size_t align, const char *name);
void union_type_add_field(type_t self, allocator_t allocator, const char *name,
                          type_t type);
void union_type_add_attribute(type_t self, allocator_t allocator,
                              const char *name, value_t value);
array_t union_type_get_fields(type_t self);
array_t union_type_get_attributes(type_t self);
union_field_t union_type_get_field(type_t self, const char *name);
value_t union_type_get_attribute(type_t self, const char *name);
#ifdef __cplusplus
}
#endif
#endif