#ifndef _H_CUBEC_CORE_ANY_
#define _H_CUBEC_CORE_ANY_
#include "core/allocator.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum _cubec_any_type_t {
  CUBEC_ANY_TYPE_NULL,
  CUBEC_ANY_TYPE_BOOLEAN,
  CUBEC_ANY_TYPE_NUMBER,
  CUBEC_ANY_TYPE_STRING,
  CUBEC_ANY_TYPE_ARRAY,
  CUBEC_ANY_TYPE_OBJECT,
} cubec_any_type_t;

typedef struct _cubec_any_t {
  cubec_any_type_t type;
  void *value;
} *cubec_any_t;

cubec_any_t cubec_create_any(cubec_allocator_t allocator);

cubec_any_t cubec_any_set_null(cubec_any_t self, cubec_allocator_t allocator);

cubec_any_t cubec_any_set_boolean(cubec_any_t self, cubec_allocator_t allocator,
                                  bool value);

cubec_any_t cubec_any_set_number(cubec_any_t self, cubec_allocator_t allocator,
                                 double value);

cubec_any_t cubec_any_set_string(cubec_any_t self, cubec_allocator_t allocator,
                                 const char *value);

cubec_any_t cubec_any_set_array(cubec_any_t self, cubec_allocator_t allocator);

cubec_any_t cubec_any_set_object(cubec_any_t self, cubec_allocator_t allocator);

bool cubec_any_get_boolean(cubec_any_t self, cubec_allocator_t allocator);

double cubec_any_get_number(cubec_any_t self, cubec_allocator_t allocator);

const char *cubec_any_get_string(cubec_any_t self, cubec_allocator_t allocator);

size_t cubec_any_get_length(cubec_any_t self);

cubec_any_t cubec_any_get_field(cubec_any_t self, cubec_allocator_t allocator,
                                const char *field);

bool cubec_any_has_field(cubec_any_t self, cubec_allocator_t allocator,
                         const char *field);

cubec_any_t cubec_any_set_field(cubec_any_t self, cubec_allocator_t allocator,
                                const char *field, cubec_any_t value);

cubec_any_t cubec_any_resize(cubec_any_t self, cubec_allocator_t allocator,
                             uint32_t size);

cubec_any_t cubec_any_get_index(cubec_any_t self, cubec_allocator_t allocator,
                                uint32_t index);

cubec_any_t cubec_any_set_index(cubec_any_t self, cubec_allocator_t allocator,
                                uint32_t index, cubec_any_t value);

cubec_any_t cubec_any_append(cubec_any_t self, cubec_allocator_t allocator,
                             cubec_any_t value);

char *cubec_any_to_json(cubec_any_t self, cubec_allocator_t allocator);

#ifdef __cplusplus
}
#endif
#endif