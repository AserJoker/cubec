#include "engine/value.h"
#include "core/allocator.h"
#include "core/array.h"
#include "core/compare.h"
#include "core/map.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
static void cubec_type_dispose(cubec_type_t self, cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->name);
}
cubec_type_t cubec_create_type(cubec_allocator_t allocator,
                               cubec_type_kind_t kind, char *name) {
  cubec_type_t type =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_type_t),
                            (cubec_dispose_fn_t)cubec_type_dispose);
  type->kind = kind;
  type->name = name;
  return type;
}
static void cubec_value_dispose(cubec_value_t self,
                                cubec_allocator_t allocator) {}

cubec_value_t cubec_create_value(cubec_allocator_t allocator,
                                 cubec_type_t type) {
  cubec_value_t value =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_value_t),
                            (cubec_dispose_fn_t)cubec_value_dispose);
  value->autofree = false;
  value->type = type;
  return value;
}

static void cubec_int8_value_dispose(cubec_int8_value_t self,
                                     cubec_allocator_t allocator) {}
cubec_type_t cubec_get_int8_type() {
  static struct _cubec_type_t type = {
      .kind = CUBEC_TYPE_KIND_INT8,
      .name = "int8",
  };
  return &type;
}
cubec_type_t cubec_get_undefined_type() {
  static struct _cubec_type_t type = {
      .kind = CUBEC_TYPE_KIND_UNDEFINED,
      .name = "undefined",
  };
  return &type;
}

cubec_value_t cubec_get_undefined() {
  static struct _cubec_value_t value = {
      .type = NULL,
      .autofree = false,
  };
  if (value.type == NULL) {
    value.type = cubec_get_undefined_type();
  }
  return &value;
}
cubec_value_t cubec_create_int8_value(cubec_allocator_t allocator,
                                      cubec_type_t type, int8_t value) {
  cubec_int8_value_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_int8_value_t),
                            (cubec_dispose_fn_t)cubec_int8_value_dispose);
  self->super.type = type;
  self->super.autofree = false;
  self->value = value;
  return &self->super;
}

cubec_type_t cubec_get_int16_type() {
  static struct _cubec_type_t type = {
      .kind = CUBEC_TYPE_KIND_INT16,
      .name = "int16",
  };
  return &type;
}

static void cubec_int16_value_dispose(cubec_int16_value_t self,
                                      cubec_allocator_t allocator) {}

cubec_value_t cubec_create_int16_value(cubec_allocator_t allocator,
                                       cubec_type_t type, int16_t value) {
  cubec_int16_value_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_int16_value_t),
                            (cubec_dispose_fn_t)cubec_int16_value_dispose);
  self->super.type = type;
  self->super.autofree = false;
  self->value = value;
  return &self->super;
}

cubec_type_t cubec_get_int32_type() {
  static struct _cubec_type_t type = {
      .kind = CUBEC_TYPE_KIND_INT32,
      .name = "int32",
  };
  return &type;
}

static void cubec_int32_value_dispose(cubec_int32_value_t self,
                                      cubec_allocator_t allocator) {}

cubec_value_t cubec_create_int32_value(cubec_allocator_t allocator,
                                       cubec_type_t type, int32_t value) {
  cubec_int32_value_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_int32_value_t),
                            (cubec_dispose_fn_t)cubec_int32_value_dispose);
  self->super.type = type;
  self->super.autofree = false;
  self->value = value;
  return &self->super;
}

cubec_type_t cubec_get_int64_type() {
  static struct _cubec_type_t type = {
      .kind = CUBEC_TYPE_KIND_INT64,
      .name = "int64",
  };
  return &type;
}

static void cubec_int64_value_dispose(cubec_int64_value_t self,
                                      cubec_allocator_t allocator) {}

cubec_value_t cubec_create_int64_value(cubec_allocator_t allocator,
                                       cubec_type_t type, int64_t value) {
  cubec_int64_value_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_int64_value_t),
                            (cubec_dispose_fn_t)cubec_int64_value_dispose);
  self->super.type = type;
  self->super.autofree = false;
  self->value = value;
  return &self->super;
}

cubec_type_t cubec_get_uint8_type() {
  static struct _cubec_type_t type = {
      .kind = CUBEC_TYPE_KIND_UINT8,
      .name = "uint8",
  };
  return &type;
}

static void cubec_uint8_value_dispose(cubec_uint8_value_t self,
                                      cubec_allocator_t allocator) {}

cubec_value_t cubec_create_uint8_value(cubec_allocator_t allocator,
                                       cubec_type_t type, uint8_t value) {
  cubec_uint8_value_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_uint8_value_t),
                            (cubec_dispose_fn_t)cubec_uint8_value_dispose);
  self->super.type = type;
  self->super.autofree = false;
  self->value = value;
  return &self->super;
}

cubec_type_t cubec_get_uint16_type() {
  static struct _cubec_type_t type = {
      .kind = CUBEC_TYPE_KIND_UINT16,
      .name = "uint16",
  };
  return &type;
}
static void cubec_uint16_value_dispose(cubec_uint16_value_t self,
                                       cubec_allocator_t allocator) {}

cubec_value_t cubec_create_uint16_value(cubec_allocator_t allocator,
                                        cubec_type_t type, uint16_t value) {
  cubec_uint16_value_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_uint16_value_t),
                            (cubec_dispose_fn_t)cubec_uint16_value_dispose);
  self->super.type = type;
  self->super.autofree = false;
  self->value = value;
  return &self->super;
}

cubec_type_t cubec_get_uint32_type() {
  static struct _cubec_type_t type = {
      .kind = CUBEC_TYPE_KIND_UINT32,
      .name = "uint32",
  };
  return &type;
}

static void cubec_uint32_value_dispose(cubec_uint32_value_t self,
                                       cubec_allocator_t allocator) {}

cubec_value_t cubec_create_uint32_value(cubec_allocator_t allocator,
                                        cubec_type_t type, uint32_t value) {
  cubec_uint32_value_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_uint32_value_t),
                            (cubec_dispose_fn_t)cubec_uint32_value_dispose);
  self->super.type = type;
  self->super.autofree = false;
  self->value = value;
  return &self->super;
}

cubec_type_t cubec_get_uint64_type() {
  static struct _cubec_type_t type = {
      .kind = CUBEC_TYPE_KIND_UINT64,
      .name = "uint64",
  };
  return &type;
}

static void cubec_uint64_value_dispose(cubec_uint64_value_t self,
                                       cubec_allocator_t allocator) {}

cubec_value_t cubec_create_uint64_value(cubec_allocator_t allocator,
                                        cubec_type_t type, uint64_t value) {
  cubec_uint64_value_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_uint64_value_t),
                            (cubec_dispose_fn_t)cubec_uint64_value_dispose);
  self->super.type = type;
  self->super.autofree = false;
  self->value = value;
  return &self->super;
}

cubec_type_t cubec_get_float32_type() {
  static struct _cubec_type_t type = {
      .kind = CUBEC_TYPE_KIND_FLOAT32,
      .name = "float32",
  };
  return &type;
}
static void cubec_float32_value_dispose(cubec_float32_value_t self,
                                        cubec_allocator_t allocator) {}
cubec_value_t cubec_create_float32_value(cubec_allocator_t allocator,
                                         cubec_type_t type, float value) {
  cubec_float32_value_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_float32_value_t),
                            (cubec_dispose_fn_t)cubec_float32_value_dispose);
  self->super.type = type;
  self->super.autofree = false;
  self->value = value;
  return &self->super;
}

cubec_type_t cubec_get_float64_type() {
  static struct _cubec_type_t type = {
      .kind = CUBEC_TYPE_KIND_FLOAT64,
      .name = "float64",
  };
  return &type;
}

static void cubec_float64_value_dispose(cubec_float64_value_t self,
                                        cubec_allocator_t allocator) {}
cubec_value_t cubec_create_float64_value(cubec_allocator_t allocator,
                                         cubec_type_t type, double value) {
  cubec_float64_value_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_float64_value_t),
                            (cubec_dispose_fn_t)cubec_float64_value_dispose);
  self->super.type = type;
  self->super.autofree = false;
  self->value = value;
  return &self->super;
}
cubec_type_t cubec_get_boolean_type() {
  static struct _cubec_type_t type = {
      .kind = CUBEC_TYPE_KIND_BOOLEAN,
      .name = "boolean",
  };
  return &type;
}
static void cubec_boolean_value_dispose(cubec_boolean_value_t self,
                                        cubec_allocator_t allocator) {}
cubec_value_t cubec_create_boolean_value(cubec_allocator_t allocator,
                                         cubec_type_t type, bool value) {
  cubec_boolean_value_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_boolean_value_t),
                            (cubec_dispose_fn_t)cubec_boolean_value_dispose);
  self->super.type = type;
  self->super.autofree = false;
  self->value = value;
  return &self->super;
}

cubec_type_t cubec_get_str_type() {
  static struct _cubec_type_t type = {
      .kind = CUBEC_TYPE_KIND_STR,
      .name = "str",
  };
  return &type;
}

static void cubec_str_value_dispose(cubec_str_value_t self,
                                    cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->value);
}
cubec_value_t cubec_create_str_value(cubec_allocator_t allocator,
                                     cubec_type_t type, char *value) {
  cubec_str_value_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_str_value_t),
                            (cubec_dispose_fn_t)cubec_str_value_dispose);
  self->super.type = type;
  self->super.autofree = false;
  self->value = value;
  return &self->super;
}
static void cubec_ptr_value_dispose(cubec_ptr_value_t self,
                                    cubec_allocator_t allocator) {
  if (self->autofree) {
    cubec_allocator_free(allocator, self->value);
  }
}
cubec_type_t cubec_get_ptr_type() {
  static struct _cubec_type_t type = {
      .kind = CUBEC_TYPE_KIND_PTR,
      .name = "ptr",
  };
  return &type;
}
cubec_value_t cubec_create_ptr_value(cubec_allocator_t allocator,
                                     cubec_type_t type, void *value,
                                     bool autofree) {
  cubec_ptr_value_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_ptr_value_t),
                            (cubec_dispose_fn_t)cubec_ptr_value_dispose);
  self->super.type = type;
  self->super.autofree = false;
  self->value = value;
  self->autofree = autofree;
  return &self->super;
}
static void cubec_ref_type_dispose(cubec_ref_type_t self,
                                   cubec_allocator_t allocator) {
  cubec_type_dispose(&self->super, allocator);
}
cubec_type_t cubec_create_ref_type(cubec_allocator_t allocator,
                                   cubec_type_t type) {
  cubec_ref_type_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_ref_type_t),
                            (cubec_dispose_fn_t)cubec_ref_type_dispose);
  self->type = type;
  self->super.kind = CUBEC_TYPE_KIND_REF;
  char *name = cubec_allocator_alloc(allocator, sizeof(type->name) + 2, NULL);
  sprintf(name, "&%s", type->name);
  self->super.name = name;
  return &self->super;
}
static void cubec_ref_value_dispose(cubec_ref_value_t self,
                                    cubec_allocator_t allocator) {}
cubec_value_t cubec_create_ref_value(cubec_allocator_t allocator,
                                     cubec_type_t type, cubec_value_t value) {
  cubec_ref_value_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_ref_value_t),
                            (cubec_dispose_fn_t)cubec_ref_value_dispose);
  self->super.type = type;
  self->super.autofree = false;
  self->value = value;
  return &self->super;
}

static void cubec_array_type_dispose(cubec_array_type_t self,
                                     cubec_allocator_t allocator) {
  cubec_type_dispose(&self->super, allocator);
}

cubec_type_t cubec_create_array_type(cubec_allocator_t allocator, char *name,
                                     cubec_type_t type, size_t len) {
  cubec_array_type_t self = cubec_allocator_alloc(
      allocator, sizeof(struct _cubec_array_type_t), NULL);
  self->type = type;
  self->length = len;
  self->super.kind = CUBEC_TYPE_KIND_ARRAY;
  if (!name) {
    char s[strlen(type->name) + 16];
    if (self->length) {
      sprintf(s, "[%" PRId64 "]%s", self->length, type->name);
    } else {
      sprintf(s, "[]%s", type->name);
    }
    size_t len = strlen(s);
    name = cubec_allocator_alloc(allocator, len + 1, NULL);
    strcpy(name, s);
    name[len] = 0;
  }
  self->super.name = name;
  return &self->super;
}

static void cubec_array_value_dispose(cubec_array_value_t self,
                                      cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->value);
  cubec_value_dispose(&self->super, allocator);
}

cubec_value_t cubec_create_array_value(cubec_allocator_t allocator,
                                       cubec_type_t type) {
  cubec_array_value_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_array_value_t),
                            (cubec_dispose_fn_t)cubec_array_value_dispose);
  cubec_array_type_t atype = (cubec_array_type_t)type;
  cubec_array_initialize_t initialize = {
      .autofree = false,
      .capacity = atype->length,
  };
  self->value = cubec_create_array(allocator, &initialize);
  for (size_t idx = 0; idx < atype->length; idx++) {
    cubec_array_set_index(self->value, allocator, idx, cubec_get_undefined());
  }
  return &self->super;
}
static void cubec_union_type_dispose(cubec_union_type_t self,
                                     cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->types);
  cubec_type_dispose(&self->super, allocator);
}
cubec_type_t cubec_create_union_type(cubec_allocator_t allocator, char *name) {
  cubec_union_type_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_union_type_t),
                            (cubec_dispose_fn_t)cubec_union_type_dispose);
  self->super.kind = CUBEC_TYPE_KIND_UNION;
  self->super.name = name;
  self->types = cubec_create_array(allocator, NULL);
  return &self->super;
}
static void cubec_union_value_dispose(cubec_union_value_t self,
                                      cubec_allocator_t allocator) {
  cubec_value_dispose(&self->super, allocator);
}
cubec_value_t cubec_create_union_value(cubec_allocator_t allocator,
                                       cubec_type_t type, cubec_value_t value) {
  cubec_union_value_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_union_value_t),
                            (cubec_dispose_fn_t)cubec_union_value_dispose);
  self->super.autofree = false;
  self->super.type = type;
  self->value = value;
  return &self->super;
}

static void cubec_struct_type_dispose(cubec_struct_type_t self,
                                      cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->attributes);
  cubec_allocator_free(allocator, self->fields);
  cubec_allocator_free(allocator, self->methods);
  cubec_type_dispose(&self->super, allocator);
}

cubec_type_t cubec_create_struct_type(cubec_allocator_t allocator, char *name) {
  cubec_struct_type_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_struct_type_t),
                            (cubec_dispose_fn_t)cubec_struct_type_dispose);
  cubec_map_initialize_t initialize = {
      .autofree_key = false,
      .autofree_value = false,
      .compare = (cubec_compare_fn_t)strcmp,
  };
  self->attributes = cubec_create_map(allocator, &initialize);
  self->fields = cubec_create_map(allocator, &initialize);
  self->methods = cubec_create_map(allocator, &initialize);
  self->super.kind = CUBEC_TYPE_KIND_STRUCT;
  self->super.name = name;
  return &self->super;
}
static void cubec_struct_value_dispose(cubec_struct_value_t self,
                                       cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->fields);
  cubec_value_dispose(&self->super, allocator);
}
cubec_value_t cubec_create_struct_value(cubec_allocator_t allocator,
                                        cubec_type_t type) {
  cubec_struct_value_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_struct_value_t),
                            (cubec_dispose_fn_t)cubec_struct_value_dispose);
  self->super.type = type;
  self->super.autofree = false;
  cubec_map_initialize_t initialize = {
      .autofree_key = false,
      .autofree_value = false,
      .compare = (cubec_compare_fn_t)strcmp,
  };
  self->fields = cubec_create_map(allocator, &initialize);
  return &self->super;
}
static void cubec_enum_type_dispose(cubec_enum_type_t self,
                                    cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->options);
  cubec_type_dispose(&self->super, allocator);
}
cubec_type_t cubec_create_enum_type(cubec_allocator_t allocator, char *name) {
  cubec_enum_type_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_enum_type_t),
                            (cubec_dispose_fn_t)cubec_enum_type_dispose);
  cubec_map_initialize_t initialize = {
      .autofree_key = false,
      .autofree_value = false,
      .compare = (cubec_compare_fn_t)strcmp,
  };
  self->options = cubec_create_map(allocator, &initialize);
  self->super.kind = CUBEC_TYPE_KIND_ENUM;
  self->super.name = name;
  return &self->super;
}
static void cubec_enum_value_dispose(cubec_enum_value_t self,
                                     cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, &self->super);
}
cubec_value_t cubec_create_enum_value(cubec_allocator_t allocator,
                                      cubec_type_t type, const char *option) {
  cubec_enum_value_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_enum_value_t),
                            (cubec_dispose_fn_t)cubec_enum_value_dispose);
  self->super.type = type;
  self->super.autofree = false;
  self->option = option;
  return &self->super;
}
static void cubec_interface_arg_dispose(cubec_interface_arg_t self,
                                        cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->name);
}
cubec_interface_arg_t cubec_create_interface_arg(cubec_allocator_t allocator,
                                                 char *name,
                                                 cubec_type_t type) {
  cubec_interface_arg_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_interface_arg_t),
                            (cubec_dispose_fn_t)cubec_interface_arg_dispose);
  self->name = name;
  self->type = type;
  return self;
}

static void cubec_interface_type_dispose(cubec_interface_type_t self,
                                         cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->args);
  cubec_allocator_free(allocator, self->closures);
  cubec_type_dispose(&self->super, allocator);
}

cubec_type_t cubec_create_interface_type(cubec_allocator_t allocator,
                                         char *name, cubec_type_t type) {
  cubec_interface_type_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_interface_type_t),
                            (cubec_dispose_fn_t)cubec_interface_type_dispose);
  self->type = type;
  self->variadic = false;
  self->super.kind = CUBEC_TYPE_KIND_INTERFACE;
  self->super.name = name;
  self->args = cubec_create_array(allocator, NULL);
  cubec_map_initialize_t initialize = {
      .autofree_key = false,
      .autofree_value = false,
      .compare = (cubec_compare_fn_t)strcmp,
  };
  self->closures = cubec_create_map(allocator, &initialize);
  return &self->super;
}

static void cubec_interface_value_dispose(cubec_interface_value_t self,
                                          cubec_allocator_t allocator) {
  cubec_value_dispose(&self->super, allocator);
}

cubec_value_t cubec_create_interface_value(cubec_allocator_t allocator,
                                           cubec_type_t type,
                                           cubec_ast_node_t node) {
  cubec_interface_value_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_struct_value_t),
                            (cubec_dispose_fn_t)cubec_interface_value_dispose);
  self->super.type = type;
  self->super.autofree = false;
  self->node = node;
  return &self->super;
}