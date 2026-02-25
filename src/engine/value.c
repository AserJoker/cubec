#include "engine/value.h"
#include "core/allocator.h"
#include "engine/type_kind.h"
cubec_int8_type_t cubec_get_type_int8() {
  static struct _cubec_type_t type = {
      .kind = CUBEC_TYPE_KIND_INT8,
      .name = "int8",
  };
  return &type;
}
cubec_int16_type_t cubec_get_type_int16() {
  static struct _cubec_type_t type = {
      .kind = CUBEC_TYPE_KIND_INT16,
      .name = "int16",
  };
  return &type;
}
cubec_int32_type_t cubec_get_type_int32() {
  static struct _cubec_type_t type = {
      .kind = CUBEC_TYPE_KIND_INT32,
      .name = "int32",
  };
  return &type;
}
cubec_int64_type_t cubec_get_type_int64() {
  static struct _cubec_type_t type = {
      .kind = CUBEC_TYPE_KIND_INT64,
      .name = "int64",
  };
  return &type;
}
cubec_uint8_type_t cubec_get_type_uint8() {
  static struct _cubec_type_t type = {
      .kind = CUBEC_TYPE_KIND_UINT8,
      .name = "uint8",
  };
  return &type;
}
cubec_uint16_type_t cubec_get_type_uint16() {
  static struct _cubec_type_t type = {
      .kind = CUBEC_TYPE_KIND_UINT16,
      .name = "uint16",
  };
  return &type;
}
cubec_uint32_type_t cubec_get_type_uint32() {
  static struct _cubec_type_t type = {
      .kind = CUBEC_TYPE_KIND_UINT32,
      .name = "uint32",
  };
  return &type;
}
cubec_uint64_type_t cubec_get_type_uint64() {
  static struct _cubec_type_t type = {
      .kind = CUBEC_TYPE_KIND_UINT64,
      .name = "uint64",
  };
  return &type;
}
cubec_float32_type_t cubec_get_type_float32() {
  static struct _cubec_type_t type = {
      .kind = CUBEC_TYPE_KIND_FLOAT32,
      .name = "float32",
  };
  return &type;
}
cubec_float64_type_t cubec_get_type_float64() {
  static struct _cubec_type_t type = {
      .kind = CUBEC_TYPE_KIND_FLOAT64,
      .name = "float64",
  };
  return &type;
}
cubec_str_type_t cubec_get_type_str() {
  static struct _cubec_type_t type = {
      .kind = CUBEC_TYPE_KIND_STR,
      .name = "str",
  };
  return &type;
}
cubec_boolean_type_t cubec_get_type_boolean() {
  static struct _cubec_type_t type = {
      .kind = CUBEC_TYPE_KIND_BOOLEAN,
      .name = "boolean",
  };
  return &type;
}
cubec_pointer_type_t cubec_get_type_pointer() {
  static struct _cubec_type_t type = {
      .kind = CUBEC_TYPE_KIND_POINTER,
      .name = "pointer",
  };
  return &type;
}
cubec_value_t cubec_create_int8_value(cubec_allocator_t allocator,
                                      int8_t value) {
  cubec_int8_value_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_int8_value_t), NULL);
  self->super.type = cubec_get_type_int8();
  self->value = value;
  return &self->super;
}
cubec_value_t cubec_create_int16_value(cubec_allocator_t allocator,
                                       int16_t value) {
  cubec_int16_value_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_int16_value_t), NULL);
  self->super.type = cubec_get_type_int16();
  self->value = value;
  return &self->super;
}
cubec_value_t cubec_create_int32_value(cubec_allocator_t allocator,
                                       int32_t value) {
  cubec_int32_value_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_int32_value_t), NULL);
  self->super.type = cubec_get_type_int32();
  self->value = value;
  return &self->super;
}
cubec_value_t cubec_create_int64_value(cubec_allocator_t allocator,
                                       int64_t value) {
  cubec_int64_value_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_int64_value_t), NULL);
  self->super.type = cubec_get_type_int64();
  self->value = value;
  return &self->super;
}

cubec_value_t cubec_create_uint8_value(cubec_allocator_t allocator,
                                       uint8_t value) {
  cubec_uint8_value_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_uint8_value_t), NULL);
  self->super.type = cubec_get_type_uint8();
  self->value = value;
  return &self->super;
}
cubec_value_t cubec_create_uint16_value(cubec_allocator_t allocator,
                                        uint16_t value) {
  cubec_uint16_value_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_uint16_value_t), NULL);
  self->super.type = cubec_get_type_uint16();
  self->value = value;
  return &self->super;
}
cubec_value_t cubec_create_uint32_value(cubec_allocator_t allocator,
                                        uint32_t value) {
  cubec_uint32_value_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_uint32_value_t), NULL);
  self->super.type = cubec_get_type_uint32();
  self->value = value;
  return &self->super;
}
cubec_value_t cubec_create_uint64_value(cubec_allocator_t allocator,
                                        uint64_t value) {
  cubec_uint64_value_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_uint64_value_t), NULL);
  self->super.type = cubec_get_type_uint64();
  self->value = value;
  return &self->super;
}
cubec_value_t cubec_create_boolean_value(cubec_allocator_t allocator,
                                         bool value) {
  cubec_boolean_value_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_boolean_value_t), NULL);
  self->super.type = cubec_get_type_boolean();
  self->value = value;
  return &self->super;
}
cubec_value_t cubec_create_float32_value(cubec_allocator_t allocator,
                                         float value) {
  cubec_float32_value_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_float32_value_t), NULL);
  self->super.type = cubec_get_type_float32();
  self->value = value;
  return &self->super;
}
cubec_value_t cubec_create_float64_value(cubec_allocator_t allocator,
                                         double value) {
  cubec_float64_value_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_float64_value_t), NULL);
  self->super.type = cubec_get_type_float64();
  self->value = value;
  return &self->super;
}
cubec_value_t cubec_create_pointer_value(cubec_allocator_t allocator,
                                         void *value) {
  cubec_pointer_value_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_pointer_value_t), NULL);
  self->super.type = cubec_get_type_pointer();
  self->value = value;
  return &self->super;
}
cubec_value_t cubec_create_str_value(cubec_allocator_t allocator,
                                     const char *value) {
  cubec_str_value_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_str_value_t), NULL);
  self->super.type = cubec_get_type_str();
  self->value = value;
  return &self->super;
}