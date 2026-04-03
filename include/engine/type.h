#ifndef _H_CUBEC_ENGINE_TYPE_
#define _H_CUBEC_ENGINE_TYPE_
#include "core/allocator.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum _cubec_type_kind_t {
  CUBEC_VALUE_TYPE_ERROR,
  CUBEC_VALUE_TYPE_ANY,
  CUBEC_VALUE_TYPE_BUILTIN,
  CUBEC_VALUE_TYPE_VOID,
  CUBEC_VALUE_TYPE_TYPE,
  CUBEC_VALUE_TYPE_BOOL,
  CUBEC_VALUE_TYPE_INT8,
  CUBEC_VALUE_TYPE_INT16,
  CUBEC_VALUE_TYPE_INT32,
  CUBEC_VALUE_TYPE_INT64,
  CUBEC_VALUE_TYPE_UINT8,
  CUBEC_VALUE_TYPE_UINT16,
  CUBEC_VALUE_TYPE_UINT32,
  CUBEC_VALUE_TYPE_UINT64,
  CUBEC_VALUE_TYPE_FLOAT16,
  CUBEC_VALUE_TYPE_FLOAT32,
  CUBEC_VALUE_TYPE_FLOAT64,
  CUBEC_VALUE_TYPE_STR,
  CUBEC_VALUE_TYPE_PTR,
  CUBEC_VALUE_TYPE_PARRAY,
  CUBEC_VALUE_TYPE_OPAQUE,
  CUBEC_VALUE_TYPE_ARRAY,
  CUBEC_VALUE_TYPE_STRUCT,
  CUBEC_VALUE_TYPE_UNION,
  CUBEC_VALUE_TYPE_FUNCTION,
} cubec_type_kind_t;
typedef struct _cubec_type_t *cubec_type_t;
typedef bool (*cubec_type_is_equal_fn_t)(cubec_type_t self,
                                         cubec_type_t another);
typedef char *(*cubec_type_to_string_fn_t)(cubec_type_t self,
                                           cubec_allocator_t allocator);
typedef struct _cubec_type_operator_t {
  cubec_type_is_equal_fn_t is_type_equal;
  cubec_type_to_string_fn_t type_to_string;
} *cubec_type_operator_t;
cubec_type_t cubec_create_type(cubec_allocator_t allocator,
                               cubec_type_kind_t kind, size_t size,
                               size_t align, void *meta,
                               cubec_type_operator_t opt);
cubec_type_kind_t cubec_type_get_kind(cubec_type_t self);
cubec_type_operator_t cubec_type_get_operator(cubec_type_t self);
size_t cubec_type_get_size(cubec_type_t self);
void cubec_type_set_size(cubec_type_t self, size_t size);
void *cubec_type_get_meta(cubec_type_t self);
size_t cubec_type_get_align(cubec_type_t self);
void cubec_type_set_align(cubec_type_t self, size_t align);
bool cubec_type_is_equal(cubec_type_t self, cubec_type_t another);
const char *cubec_type_kind_to_string(cubec_type_kind_t kind);
char *cubec_type_to_string(cubec_type_t self, cubec_allocator_t allocator);
#ifdef __cplusplus
}
#endif
#endif