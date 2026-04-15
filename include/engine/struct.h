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

struct _struct_attribute_t {
  char *name;
  value_t value;
};
typedef struct _struct_attribute_t *struct_attribute_t;
struct _struct_field_t {
  char *name;
  type_t type;
  size_t offset;
};
typedef struct _struct_field_t *struct_field_t;
value_t create_struct_type(context_t ctx, size_t align, const char *name);
void struct_type_add_field(type_t self, allocator_t allocator, const char *name,
                           type_t type);
void struct_type_add_attribute(type_t self, allocator_t allocator,
                               const char *name, value_t value);
type_t struct_type_get_field(type_t self, const char *name);
value_t struct_type_get_attribute(type_t self, const char *name);
array_t struct_type_get_fields(type_t self);
array_t struct_type_get_attributes(type_t self);

#ifdef __cplusplus
}
#endif
#endif