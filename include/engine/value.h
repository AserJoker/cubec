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
typedef enum _cubec_type_kind_t {
  CUBEC_TYPE_KIND_ERROR,
  CUBEC_TYPE_KIND_UNDEFINED,
  CUBEC_TYPE_KIND_INT8,
  CUBEC_TYPE_KIND_INT16,
  CUBEC_TYPE_KIND_INT32,
  CUBEC_TYPE_KIND_INT64,
  CUBEC_TYPE_KIND_UINT8,
  CUBEC_TYPE_KIND_UINT16,
  CUBEC_TYPE_KIND_UINT32,
  CUBEC_TYPE_KIND_UINT64,
  CUBEC_TYPE_KIND_FLOAT32,
  CUBEC_TYPE_KIND_FLOAT64,
  CUBEC_TYPE_KIND_BOOLEAN,
  CUBEC_TYPE_KIND_STR,
  CUBEC_TYPE_KIND_PTR,
  CUBEC_TYPE_KIND_REF,
  CUBEC_TYPE_KIND_ARRAY,
  CUBEC_TYPE_KIND_STRUCT,
  CUBEC_TYPE_KIND_UNION,
  CUBEC_TYPE_KIND_ENUM,
  CUBEC_TYPE_KIND_INTERFACE,
} cubec_type_kind_t;
typedef struct _cubec_type_t *cubec_type_t;

struct _cubec_type_t {
  cubec_type_kind_t kind;
  char *name;
};

cubec_type_t cubec_create_type(cubec_allocator_t allocator,
                               cubec_type_kind_t kind, char *name);

typedef struct _cubec_value_t *cubec_value_t;

struct _cubec_value_t {
  cubec_type_t type;
  bool autofree;
};

cubec_value_t cubec_create_value(cubec_allocator_t allocator,
                                 cubec_type_t type);

cubec_type_t cubec_get_undefined_type();

cubec_value_t cubec_get_undefined();

cubec_type_t cubec_get_int8_type();

typedef struct _cubec_int8_value_t *cubec_int8_value_t;
struct _cubec_int8_value_t {
  struct _cubec_value_t super;
  int8_t value;
};

cubec_value_t cubec_create_int8_value(cubec_allocator_t allocator,
                                      cubec_type_t type, int8_t value);

cubec_type_t cubec_get_int16_type();

typedef struct _cubec_int16_value_t *cubec_int16_value_t;
struct _cubec_int16_value_t {
  struct _cubec_value_t super;
  int16_t value;
};

cubec_value_t cubec_create_int16_value(cubec_allocator_t allocator,
                                       cubec_type_t type, int16_t value);

cubec_type_t cubec_get_int32_type();

typedef struct _cubec_int32_value_t *cubec_int32_value_t;
struct _cubec_int32_value_t {
  struct _cubec_value_t super;
  int32_t value;
};

cubec_value_t cubec_create_int32_value(cubec_allocator_t allocator,
                                       cubec_type_t type, int32_t value);

cubec_type_t cubec_get_int64_type();

typedef struct _cubec_int64_value_t *cubec_int64_value_t;
struct _cubec_int64_value_t {
  struct _cubec_value_t super;
  int64_t value;
};

cubec_value_t cubec_create_int64_value(cubec_allocator_t allocator,
                                       cubec_type_t type, int64_t value);
typedef struct _cubec_uint8_value_t *cubec_uint8_value_t;
struct _cubec_uint8_value_t {
  struct _cubec_value_t super;
  uint8_t value;
};

cubec_type_t cubec_get_uint8_type();

cubec_value_t cubec_create_uint8_value(cubec_allocator_t allocator,
                                       cubec_type_t type, uint8_t value);

typedef struct _cubec_uint16_value_t *cubec_uint16_value_t;
struct _cubec_uint16_value_t {
  struct _cubec_value_t super;
  uint16_t value;
};

cubec_type_t cubec_get_uint16_type();

cubec_value_t cubec_create_uint16_value(cubec_allocator_t allocator,
                                        cubec_type_t type, uint16_t value);

typedef struct _cubec_uint32_value_t *cubec_uint32_value_t;
struct _cubec_uint32_value_t {
  struct _cubec_value_t super;
  uint32_t value;
};

cubec_type_t cubec_get_uint32_type();

cubec_value_t cubec_create_uint32_value(cubec_allocator_t allocator,
                                        cubec_type_t type, uint32_t value);

typedef struct _cubec_uint64_value_t *cubec_uint64_value_t;
struct _cubec_uint64_value_t {
  struct _cubec_value_t super;
  uint64_t value;
};

cubec_type_t cubec_get_uint64_type();

cubec_value_t cubec_create_uint64_value(cubec_allocator_t allocator,
                                        cubec_type_t type, uint64_t value);

typedef struct _cubec_uint32_value_t *cubec_float32_value_t;
struct _cubec_float32_value_t {
  struct _cubec_value_t super;
  float value;
};

cubec_type_t cubec_get_float32_type();

cubec_value_t cubec_create_float32_value(cubec_allocator_t allocator,
                                         cubec_type_t type, float value);

typedef struct _cubec_uint64_value_t *cubec_float64_value_t;
struct _cubec_float64_value_t {
  struct _cubec_value_t super;
  double value;
};

cubec_type_t cubec_get_float64_type();

cubec_value_t cubec_create_float64_value(cubec_allocator_t allocator,
                                         cubec_type_t type, double value);

typedef struct _cubec_boolean_value_t *cubec_boolean_value_t;
struct _cubec_boolean_value_t {
  struct _cubec_value_t super;
  bool value;
};
cubec_type_t cubec_get_boolean_type();
cubec_value_t cubec_create_boolean_value(cubec_allocator_t allocator,
                                         cubec_type_t type, bool value);

typedef struct _cubec_str_value_t *cubec_str_value_t;
struct _cubec_str_value_t {
  struct _cubec_value_t super;
  char *value;
};

cubec_type_t cubec_get_str_type();
cubec_value_t cubec_create_str_value(cubec_allocator_t allocator,
                                     cubec_type_t type, char *value);

typedef struct _cubec_ptr_value_t *cubec_ptr_value_t;
struct _cubec_ptr_value_t {
  struct _cubec_value_t super;
  void *value;
  bool autofree;
};

cubec_type_t cubec_get_str_type();
cubec_value_t cubec_create_ptr_value(cubec_allocator_t allocator,
                                     cubec_type_t type, void *value,
                                     bool autofree);
typedef struct _cubec_ref_type_t *cubec_ref_type_t;
struct _cubec_ref_type_t {
  struct _cubec_type_t super;
  cubec_type_t type;
};
cubec_type_t cubec_create_ref_type(cubec_allocator_t allocator,
                                   cubec_type_t type);
typedef struct _cubec_ref_value_t *cubec_ref_value_t;
struct _cubec_ref_value_t {
  struct _cubec_value_t super;
  cubec_value_t value;
};

cubec_value_t cubec_create_ref_value(cubec_allocator_t allocator,
                                     cubec_type_t type, cubec_value_t value);

typedef struct _cubec_array_type_t *cubec_array_type_t;
struct _cubec_array_type_t {
  struct _cubec_type_t super;
  size_t length;
  cubec_type_t type;
};

cubec_type_t cubec_create_array_type(cubec_allocator_t allocator, char *name,
                                     cubec_type_t type, size_t len);

typedef struct _cubec_array_value_t *cubec_array_value_t;
struct _cubec_array_value_t {
  struct _cubec_value_t super;
  cubec_array_t value;
};

cubec_value_t cubec_create_array_value(cubec_allocator_t allocator,
                                       cubec_type_t type);

typedef struct _cubec_union_type_t *cubec_union_type_t;
struct _cubec_union_type_t {
  struct _cubec_type_t super;
  cubec_array_t types;
};
cubec_type_t cubec_create_union_type(cubec_allocator_t allocator, char *name);

typedef struct _cubec_union_value_t *cubec_union_value_t;
struct _cubec_union_value_t {
  struct _cubec_value_t super;
  cubec_value_t value;
};

cubec_value_t cubec_create_union_value(cubec_allocator_t allocator,
                                       cubec_type_t type, cubec_value_t value);
typedef struct _cubec_struct_type_t *cubec_struct_type_t;
struct _cubec_struct_type_t {
  struct _cubec_type_t super;
  cubec_map_t methods;
  cubec_map_t attributes;
  cubec_map_t fields;
};
cubec_type_t cubec_create_struct_type(cubec_allocator_t allocator, char *name);
typedef struct _cubec_struct_value_t *cubec_struct_value_t;
struct _cubec_struct_value_t {
  struct _cubec_value_t super;
  cubec_map_t fields;
};

cubec_value_t cubec_create_struct_value(cubec_allocator_t allocator,
                                        cubec_type_t type);

typedef struct _cubec_enum_type_t *cubec_enum_type_t;
struct _cubec_enum_type_t {
  struct _cubec_type_t super;
  cubec_map_t options;
};

cubec_type_t cubec_create_enum_type(cubec_allocator_t allocator, char *name);

typedef struct _cubec_enum_value_t *cubec_enum_value_t;
struct _cubec_enum_value_t {
  struct _cubec_value_t super;
  const char *option;
};
cubec_value_t cubec_create_enum_value(cubec_allocator_t allocator,
                                      cubec_type_t type, const char *option);

typedef struct _cubec_interface_arg_t *cubec_interface_arg_t;
struct _cubec_interface_arg_t {
  char *name;
  cubec_type_t type;
};
cubec_interface_arg_t cubec_create_interface_arg(cubec_allocator_t allocator,
                                                 char *name, cubec_type_t type);

typedef struct _cubec_interface_type_t *cubec_interface_type_t;
struct _cubec_interface_type_t {
  struct _cubec_type_t super;
  cubec_type_t type;
  cubec_array_t args;
  bool variadic;
  cubec_map_t closures;
};

cubec_type_t cubec_create_interface_type(cubec_allocator_t allocator,
                                         char *name, cubec_type_t type);

typedef struct _cubec_interface_value_t *cubec_interface_value_t;
struct _cubec_interface_value_t {
  struct _cubec_value_t super;
  cubec_ast_node_t node;
};

cubec_value_t cubec_create_interface_value(cubec_allocator_t allocator,
                                           cubec_type_t type,
                                           cubec_ast_node_t node);
#ifdef __cplusplus
}
#endif
#endif