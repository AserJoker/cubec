#ifndef _H_CUBEC_ENGINE_TYPE_
#define _H_CUBEC_ENGINE_TYPE_
#include "core/allocator.h"
#include <stdbool.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum _cubec_type_kind_t {
  CUBEC_TYPE_KIND_ERROR,
  CUBEC_TYPE_KIND_VOID,
  CUBEC_TYPE_KIND_INT8,
  CUBEC_TYPE_KIND_INT16,
  CUBEC_TYPE_KIND_INT32,
  CUBEC_TYPE_KIND_INT64,
  CUBEC_TYPE_KIND_UINT8,
  CUBEC_TYPE_KIND_UINT16,
  CUBEC_TYPE_KIND_UINT32,
  CUBEC_TYPE_KIND_UINT64,
  CUBEC_TYPE_KIND_BOOLEAN,
  CUBEC_TYPE_KIND_STR,
  CUBEC_TYPE_KIND_OPAQUE,
  CUBEC_TYPE_KIND_PTR,
  CUBEC_TYPE_KIND_PTR_ARRAY,
  CUBEC_TYPE_KIND_ARRAY,
  CUBEC_TYPE_KIND_STRUCT,
  CUBEC_TYPE_KIND_UNION,
  CUBEC_TYPE_KIND_ENUM,
  CUBEC_TYPE_KIND_RESULT,
  CUBEC_TYPE_KIND_FUNCTION,
  CUBEC_TYPE_KIND_TEMPLATE,
} cubec_type_kind_t;
typedef struct _cubec_type_t *cubec_type_t;
struct _cubec_type_t {
  cubec_type_kind_t kind;
  size_t size;
  void *meta;
};
cubec_type_t cubec_create_type(cubec_allocator_t allocator,
                               cubec_type_kind_t kind, size_t size, void *meta);
#ifdef __cplusplus
}
#endif
#endif