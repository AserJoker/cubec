#ifndef _H_CUBEC_CORE_VALUE_
#define _H_CUBEC_CORE_VALUE_
#include "core/allocator.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum _cubec_value_type_t {
  CUBEC_VALUE_TYPE_NULL,
  CUBEC_VALUE_TYPE_BOOLEAN,
  CUBEC_VALUE_TYPE_NUMBER,
  CUBEC_VALUE_TYPE_STRING,
  CUBEC_VALUE_TYPE_ARRAY,
  CUBEC_VALUE_TYPE_OBJECT,
} cubec_value_type_t;

typedef struct _cubec_value_t {
  cubec_value_type_t type;
  void *value;
} *cubec_value_t;

cubec_value_t cubec_create_value(cubec_allocator_t allocator);

cubec_value_t cubec_value_set_null(cubec_value_t self,
                                   cubec_allocator_t allocator);

cubec_value_t cubec_value_set_boolean(cubec_value_t self,
                                      cubec_allocator_t allocator, bool value);

cubec_value_t cubec_value_set_number(cubec_value_t self,
                                     cubec_allocator_t allocator, double value);

cubec_value_t cubec_value_set_string(cubec_value_t self,
                                     cubec_allocator_t allocator,
                                     const char *value);

cubec_value_t cubec_value_set_array(cubec_value_t self,
                                    cubec_allocator_t allocator);

cubec_value_t cubec_value_set_object(cubec_value_t self,
                                     cubec_allocator_t allocator);

bool cubec_value_get_boolean(cubec_value_t self, cubec_allocator_t allocator);

double cubec_value_get_number(cubec_value_t self, cubec_allocator_t allocator);

const char *cubec_value_get_string(cubec_value_t self,
                                   cubec_allocator_t allocator);

size_t cubec_value_get_length(cubec_value_t self);

cubec_value_t cubec_value_get_field(cubec_value_t self,
                                    cubec_allocator_t allocator,
                                    const char *field);

bool cubec_value_has_field(cubec_value_t self, cubec_allocator_t allocator,
                           const char *field);

cubec_value_t cubec_value_set_field(cubec_value_t self,
                                    cubec_allocator_t allocator,
                                    const char *field, cubec_value_t value);

cubec_value_t cubec_value_resize(cubec_value_t self,
                                 cubec_allocator_t allocator, uint32_t size);

cubec_value_t cubec_value_get_index(cubec_value_t self,
                                    cubec_allocator_t allocator,
                                    uint32_t index);

cubec_value_t cubec_value_set_index(cubec_value_t self,
                                    cubec_allocator_t allocator, uint32_t index,
                                    cubec_value_t value);

cubec_value_t cubec_value_append(cubec_value_t self,
                                 cubec_allocator_t allocator,
                                 cubec_value_t value);

#ifdef __cplusplus
}
#endif
#endif