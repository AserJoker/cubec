#ifndef _H_CUBEC_ENGINE_TYPE_
#define _H_CUBEC_ENGINE_TYPE_
#include "core/allocator.h"
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum _cubec_type_kind_t {
  CUBEC_VALUE_TYPE_ERROR = -1,
  CUBEC_VALUE_TYPE_UNDEFINED,
  CUBEC_VALUE_TYPE_INT8,
  CUBEC_VALUE_TYPE_INT16,
  CUBEC_VALUE_TYPE_INT32,
  CUBEC_VALUE_TYPE_INT64,
  CUBEC_VALUE_TYPE_UINT8,
  CUBEC_VALUE_TYPE_UINT16,
  CUBEC_VALUE_TYPE_UINT32,
  CUBEC_VALUE_TYPE_UINT64,
  CUBEC_VALUE_TYPE_FLOAT32,
  CUBEC_VALUE_TYPE_FLOAT64,
  CUBEC_VALUE_TYPE_BOOLEAN,
  CUBEC_VALUE_TYPE_STR,
  CUBEC_VALUE_TYPE_OPAQUE,
  CUBEC_VALUE_TYPE_PTR,
  CUBEC_VALUE_TYPE_PTR_ARRAY,
  CUBEC_VALUE_TYPE_REF,
  CUBEC_VALUE_TYPE_STRUCT,
  CUBEC_VALUE_TYPE_ARRAY,
  CUBEC_VALUE_TYPE_FUNCTION,
  CUBEC_VALUE_TYPE_ENUM,
  CUBEC_VALUE_TYPE_UNION,
} cubec_type_kind_t;

typedef struct _cubec_type_t *cubec_type_t;
struct _cubec_type_t {
  cubec_type_kind_t kind;
  size_t size;
  char *name;
  void *meta;
};
cubec_type_t cubec_create_type(cubec_allocator_t allocator,
                               cubec_type_kind_t kind, size_t size,
                               const char *name, void *meta);
#ifdef __cplusplus
}
#endif
#endif