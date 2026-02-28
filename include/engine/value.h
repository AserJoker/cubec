#ifndef _H_CUBEC_ENGINE_VALUE_
#define _H_CUBEC_ENGINE_VALUE_
#include "ast/node.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/map.h"
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum _cubec_value_type_t {
  CUBEC_VALUE_TYPE_ERROR,
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
  CUBEC_VALUE_TYPE_PTR,
  CUBEC_VALUE_TYPE_REF,
  CUBEC_VALUE_TYPE_ARRAY,
  CUBEC_VALUE_TYPE_STRUCT,
  CUBEC_VALUE_TYPE_FUNCTION,
  CUBEC_VALUE_TYPE_NATIVE,
} cubec_value_type_t;
typedef struct _cubec_value_t *cubec_value_t;
struct _cubec_value_t {
  cubec_value_type_t kind;
  bool autofree;
  void *data;
};

cubec_value_t cubec_create_value(cubec_allocator_t allocator,
                                 cubec_value_type_t type, bool autofree,
                                 void *data);

typedef cubec_value_t cubec_undefined_t;
cubec_value_t cubec_create_undefined_value(cubec_allocator_t allocator,
                                           bool autofree);

typedef struct _cubec_error_data_t *cubec_error_data_t;
struct _cubec_error_data_t {
  cubec_value_t error;
};
cubec_value_t cubec_create_error_value(cubec_allocator_t allocator,
                                       cubec_value_t error);

#define DECLAR_VALUE_TYPE(name, ctype)                                         \
  typedef struct _cubec_##name##_data_t *cubec_##name##_data_t;                \
  struct _cubec_##name##_data_t {                                              \
    ctype value;                                                               \
  };                                                                           \
  cubec_value_t cubec_create_##name##_value(cubec_allocator_t allocator,       \
                                            bool autofree, ctype value);

DECLAR_VALUE_TYPE(int8, int8_t)
DECLAR_VALUE_TYPE(int16, int16_t)
DECLAR_VALUE_TYPE(int32, int32_t)
DECLAR_VALUE_TYPE(int64, int64_t)
DECLAR_VALUE_TYPE(uint8, uint8_t)
DECLAR_VALUE_TYPE(uint16, uint16_t)
DECLAR_VALUE_TYPE(uint32, uint32_t)
DECLAR_VALUE_TYPE(uint64, uint64_t)
DECLAR_VALUE_TYPE(float32, float)
DECLAR_VALUE_TYPE(float64, double)
DECLAR_VALUE_TYPE(boolean, bool)
DECLAR_VALUE_TYPE(str, char *)
DECLAR_VALUE_TYPE(ref, cubec_value_t)
typedef struct _cubec_ptr_data_t *cubec_ptr_data_t;
struct _cubec_ptr_data_t {
  void *value;
  bool autofree;
};
cubec_value_t cubec_create_ptr_value(cubec_allocator_t allocator, bool autofree,
                                     void *value, bool autofree_value);

typedef struct _cubec_array_data_t *cubec_array_data_t;
struct _cubec_array_data_t {
  size_t capacity;
  cubec_array_t value;
  cubec_value_t type;
};

cubec_value_t cubec_create_array_value(cubec_allocator_t allocator,
                                       bool autofree, size_t capacity,
                                       cubec_value_t type);

typedef struct _cubec_struct_data_t *cubec_struct_data_t;
struct _cubec_struct_data_t {
  cubec_map_t fields;
  cubec_value_t type;
};
cubec_value_t cubec_create_struct_value(cubec_allocator_t allocator,
                                        bool autofree, cubec_value_t type);

typedef struct _cubec_function_data_t *cubec_function_data_t;
struct _cubec_function_data_t {
  cubec_value_t type;
  cubec_map_t closures;
  cubec_ast_node_t node;
};

cubec_value_t cubec_create_function_value(cubec_allocator_t allocator,
                                          bool autofree, cubec_value_t type,
                                          cubec_ast_node_t node);

struct _cubec_context_t;
typedef cubec_value_t (*cubec_native_fn_t)(struct _cubec_context_t *ctx,
                                           size_t argc, cubec_value_t *argv);

typedef struct _cubec_native_data_t *cubec_native_data_t;
struct _cubec_native_data_t {
  cubec_value_t type;
  cubec_map_t closures;
  cubec_native_fn_t value;
};
cubec_value_t cubec_create_native_value(cubec_allocator_t allocator,
                                        bool autofree, cubec_value_t type,
                                        cubec_native_fn_t value);

#ifdef __cplusplus
}
#endif
#endif