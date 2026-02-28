#include "engine/value.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/compare.h"
#include "core/map.h"
#include <stdbool.h>
#include <string.h>
static void cubec_value_dispose(cubec_value_t self,
                                cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->data);
}
cubec_value_t cubec_create_value(cubec_allocator_t allocator,
                                 cubec_value_type_t type, bool autofree,
                                 void *data) {
  cubec_value_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_value_t),
                            (cubec_dispose_fn_t)cubec_value_dispose);
  self->autofree = autofree;
  self->data = data;
  self->kind = type;
  return self;
}

cubec_value_t cubec_create_undefined_value(cubec_allocator_t allocator,
                                           bool autofree) {
  return cubec_create_value(allocator, CUBEC_VALUE_TYPE_UNDEFINED, autofree,
                            NULL);
}
cubec_value_t cubec_create_error_value(cubec_allocator_t allocator,
                                       cubec_value_t error) {
  cubec_error_data_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_error_data_t), NULL);
  self->error = error;
  return cubec_create_value(allocator, CUBEC_VALUE_TYPE_ERROR, false, self);
}
cubec_value_t cubec_create_int8_value(cubec_allocator_t allocator,
                                      bool autofree, int8_t value) {
  cubec_int8_data_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_int8_data_t), NULL);
  self->value = value;
  return cubec_create_value(allocator, CUBEC_VALUE_TYPE_INT8, autofree, self);
}
cubec_value_t cubec_create_int16_value(cubec_allocator_t allocator,
                                       bool autofree, int16_t value) {
  cubec_int16_data_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_int16_data_t), NULL);
  self->value = value;
  return cubec_create_value(allocator, CUBEC_VALUE_TYPE_INT16, autofree, self);
}
cubec_value_t cubec_create_int32_value(cubec_allocator_t allocator,
                                       bool autofree, int32_t value) {
  cubec_int32_data_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_int32_data_t), NULL);
  self->value = value;
  return cubec_create_value(allocator, CUBEC_VALUE_TYPE_INT32, autofree, self);
}
cubec_value_t cubec_create_int64_value(cubec_allocator_t allocator,
                                       bool autofree, int64_t value) {
  cubec_int64_data_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_int64_data_t), NULL);
  self->value = value;
  return cubec_create_value(allocator, CUBEC_VALUE_TYPE_INT64, autofree, self);
}

cubec_value_t cubec_create_uint8_value(cubec_allocator_t allocator,
                                       bool autofree, uint8_t value) {
  cubec_uint8_data_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_uint8_data_t), NULL);
  self->value = value;
  return cubec_create_value(allocator, CUBEC_VALUE_TYPE_UINT8, autofree, self);
}
cubec_value_t cubec_create_uint16_value(cubec_allocator_t allocator,
                                        bool autofree, uint16_t value) {
  cubec_uint16_data_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_uint16_data_t), NULL);
  self->value = value;
  return cubec_create_value(allocator, CUBEC_VALUE_TYPE_UINT16, autofree, self);
}
cubec_value_t cubec_create_uint32_value(cubec_allocator_t allocator,
                                        bool autofree, uint32_t value) {
  cubec_uint32_data_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_uint32_data_t), NULL);
  self->value = value;
  return cubec_create_value(allocator, CUBEC_VALUE_TYPE_UINT32, autofree, self);
}
cubec_value_t cubec_create_uint64_value(cubec_allocator_t allocator,
                                        bool autofree, uint64_t value) {
  cubec_uint64_data_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_uint64_data_t), NULL);
  self->value = value;
  return cubec_create_value(allocator, CUBEC_VALUE_TYPE_UINT64, autofree, self);
}
cubec_value_t cubec_create_float32_value(cubec_allocator_t allocator,
                                         bool autofree, float value) {
  cubec_float32_data_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_float32_data_t), NULL);
  self->value = value;
  return cubec_create_value(allocator, CUBEC_VALUE_TYPE_FLOAT32, autofree,
                            self);
}
cubec_value_t cubec_create_float64_value(cubec_allocator_t allocator,
                                         bool autofree, double value) {
  cubec_float64_data_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_float64_data_t), NULL);
  self->value = value;
  return cubec_create_value(allocator, CUBEC_VALUE_TYPE_FLOAT64, autofree,
                            self);
}
cubec_value_t cubec_create_boolean_value(cubec_allocator_t allocator,
                                         bool autofree, bool value) {
  cubec_boolean_data_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_boolean_data_t), NULL);
  self->value = value;
  return cubec_create_value(allocator, CUBEC_VALUE_TYPE_BOOLEAN, autofree,
                            self);
}
static void cubec_str_data_dispose(cubec_str_data_t self,
                                   cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->value);
}

cubec_value_t cubec_create_str_value(cubec_allocator_t allocator, bool autofree,
                                     char *value) {
  cubec_str_data_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_str_data_t),
                            (cubec_dispose_fn_t)cubec_str_data_dispose);
  self->value = value;
  return cubec_create_value(allocator, CUBEC_VALUE_TYPE_STR, autofree, self);
}

static void cubec_ptr_data_dispose(cubec_ptr_data_t self,
                                   cubec_allocator_t allocator) {
  if (self->autofree) {
    cubec_allocator_free(allocator, self->value);
  }
}

cubec_value_t cubec_create_ptr_value(cubec_allocator_t allocator, bool autofree,
                                     void *value, bool autofree_value) {
  cubec_ptr_data_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_ptr_data_t),
                            (cubec_dispose_fn_t)cubec_ptr_data_dispose);
  self->value = value;
  self->autofree = autofree_value;
  return cubec_create_value(allocator, CUBEC_VALUE_TYPE_PTR, autofree, self);
}
cubec_value_t cubec_create_ref_value(cubec_allocator_t allocator, bool autofree,
                                     cubec_value_t value) {
  cubec_ref_data_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_ref_data_t), NULL);
  self->value = value;
  return cubec_create_value(allocator, CUBEC_VALUE_TYPE_REF, autofree, self);
}

static void cubec_array_data_dispsoe(cubec_array_data_t self,
                                     cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->value);
}

cubec_value_t cubec_create_array_value(cubec_allocator_t allocator,
                                       bool autofree, size_t capacity,
                                       cubec_value_t type) {
  cubec_array_data_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_array_data_t),
                            (cubec_dispose_fn_t)cubec_array_data_dispsoe);
  self->capacity = capacity;
  self->type = type;
  self->value = cubec_create_array(allocator, NULL);
  return cubec_create_value(allocator, CUBEC_VALUE_TYPE_ARRAY, autofree, self);
}

static void cubec_struct_data_dispose(cubec_struct_data_t self,
                                      cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->fields);
}

cubec_value_t cubec_create_struct_value(cubec_allocator_t allocator,
                                        bool autofree, cubec_value_t type) {
  cubec_struct_data_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_struct_data_t),
                            (cubec_dispose_fn_t)cubec_struct_data_dispose);
  self->type = type;
  cubec_map_initialize_t initialize = {
      .autofree_key = false,
      .autofree_value = true,
      .compare = (cubec_compare_fn_t)strcmp,
  };
  self->fields = cubec_create_map(allocator, &initialize);
  return cubec_create_value(allocator, CUBEC_VALUE_TYPE_STRUCT, autofree, self);
}

cubec_value_t cubec_create_function_value(cubec_allocator_t allocator,
                                          bool autofree, cubec_value_t type,
                                          cubec_ast_node_t node) {
  cubec_function_data_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_function_data_t),
                            (cubec_dispose_fn_t)NULL);
  self->type = type;
  self->node = node;
  cubec_map_initialize_t initialize = {
      .autofree_key = true,
      .autofree_value = false,
      .compare = (cubec_compare_fn_t)strcmp,
  };
  self->closures = cubec_create_map(allocator, &initialize);
  return cubec_create_value(allocator, CUBEC_VALUE_TYPE_FUNCTION, autofree,
                            self);
}

cubec_value_t cubec_create_native_value(cubec_allocator_t allocator,
                                        bool autofree, cubec_value_t type,
                                        cubec_native_fn_t value) {
  cubec_native_data_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_native_data_t), (cubec_dispose_fn_t)NULL);
  self->type = type;
  self->value = value;
  cubec_map_initialize_t initialize = {
      .autofree_key = true,
      .autofree_value = false,
      .compare = (cubec_compare_fn_t)strcmp,
  };
  self->closures = cubec_create_map(allocator, &initialize);
  return cubec_create_value(allocator, CUBEC_VALUE_TYPE_NATIVE, autofree, self);
}