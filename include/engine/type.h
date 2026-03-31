#ifndef _H_CUBEC_ENGINE_TYPE_
#define _H_CUBEC_ENGINE_TYPE_
#include "core/allocator.h"
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum _cubec_type_kind_t {
  CUBEC_VALUE_TYPE_ERROR,
  CUBEC_VALUE_TYPE_ANY,
  CUBEC_VALUE_TYPE_TYPE,
  CUBEC_VALUE_TYPE_MODULE,
  CUBEC_VALUE_TYPE_VOID = 0,
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
  CUBEC_VALUE_TYPE_PTR_ARRAY,
  CUBEC_VALUE_TYPE_OPAQUE,
  CUBEC_VALUE_TYPE_ARRAY,
  CUBEC_VALUE_TYPE_STRUCT,
  CUBEC_VALUE_TYPE_UNION,
  CUBEC_VALUE_TYPE_RESULT,
  CUBEC_VALUE_TYPE_OPTIONAL,
  CUBEC_VALUE_TYPE_FUNCTION,
} cubec_type_kind_t;
typedef struct _cubec_type_t *cubec_type_t;
cubec_type_t cubec_create_type(cubec_allocator_t allocator,
                               cubec_type_kind_t kind, size_t size, void *meta);
cubec_type_kind_t cubec_type_get_kind(cubec_type_t self);
size_t cubec_type_get_size(cubec_type_t self);
void cubec_type_set_size(cubec_type_t self, size_t size);
void *cubec_type_get_meta(cubec_type_t self);
#ifdef __cplusplus
}
#endif
#endif