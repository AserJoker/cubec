#include "engine/value.h"
#include "core/allocator.h"
#include "engine/context.h"
#include "engine/error.h"
#include "engine/str.h"
#include "engine/type.h"
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
struct _cubec_value_t {
  cubec_type_t type;
  bool mutable;
  void *data;
};
static void cubec_value_dispose(cubec_value_t self,
                                cubec_allocator_t allocator) {
  cubec_allocator_free(allocator, self->data);
}
cubec_value_t cubec_create_value(cubec_allocator_t allocator, cubec_type_t type,
                                 bool mutable, const void *data) {
  cubec_value_t self =
      cubec_allocator_alloc(allocator, sizeof(struct _cubec_value_t),
                            (cubec_dispose_fn_t)cubec_value_dispose);
  size_t size = cubec_type_get_size(type);
  self->mutable = mutable;
  self->type = type;
  if (data) {
    self->data = cubec_allocator_alloc(allocator, size, NULL);
    memcpy(self->data, data, size);
  } else {
    self->data = NULL;
  }
  return self;
}
cubec_type_t cubec_value_get_type(cubec_value_t value) { return value->type; }
bool cubec_value_type_is(cubec_value_t value, cubec_type_kind_t kind) {
  return cubec_type_get_kind(value->type) == kind;
}
bool cubec_value_is_mutable(cubec_value_t value) { return value->mutable; }
void cubec_value_set_mutable(cubec_value_t value, bool mutable) {
  value->mutable = mutable;
}
void *cubec_value_get_data(cubec_value_t value) { return value->data; }
cubec_value_t cubec_value_clone(cubec_allocator_t allocator,
                                cubec_value_t value) {
  return cubec_create_value(allocator, value->type, value->mutable,
                            value->type);
}
bool cubec_value_is_error(cubec_value_t value) {
  cubec_type_t type = cubec_value_get_type(value);
  cubec_type_kind_t kind = cubec_type_get_kind(type);
  return kind == CUBEC_VALUE_TYPE_ERROR;
}

cubec_value_t cubec_value_to_string(cubec_value_t self, cubec_context_t ctx) {
  cubec_type_t type = cubec_value_get_type(self);
  cubec_type_operator_t opt = cubec_type_get_operator(type);
  if (opt->to_string) {
    return opt->to_string(self, ctx);
  }
  cubec_allocator_t allocator = cubec_context_get_allocator(ctx);
  char *type_name = cubec_type_to_string(type, allocator);
  void *data = cubec_value_get_data(self);
  size_t len =
      snprintf(NULL, 0, "%s{0x%" PRIXPTR "}", type_name, (intptr_t)data);
  char str[len + 1];
  sprintf(str, "%s{0x%" PRIXPTR "}", type_name, (intptr_t)data);
  cubec_allocator_free(allocator, type_name);
  return cubec_create_str(ctx, str, NULL);
}
cubec_value_t cubec_value_get_index(cubec_value_t self, cubec_context_t ctx,
                                    size_t idx) {
  cubec_type_t type = cubec_value_get_type(self);
  cubec_type_operator_t opt = cubec_type_get_operator(type);
  if (opt->get_index) {
    return opt->get_index(self, ctx, idx);
  }
  return cubec_create_error(ctx, "Value does not support index access");
}
cubec_value_t cubec_value_set_index(cubec_value_t self,
                                    struct _cubec_context_t *ctx, size_t idx,
                                    cubec_value_t item) {
  cubec_type_t type = cubec_value_get_type(self);
  cubec_type_operator_t opt = cubec_type_get_operator(type);
  if (opt->set_index) {
    return opt->set_index(self, ctx, idx, item);
  }
  return cubec_create_error(ctx, "Value does not support index access");
}
cubec_value_t cubec_value_get_field(cubec_value_t self, cubec_context_t ctx,
                                    const char *name) {
  cubec_type_t type = cubec_value_get_type(self);
  cubec_type_operator_t opt = cubec_type_get_operator(type);
  if (opt->get_field) {
    return opt->get_field(self, ctx, name);
  }
  return cubec_create_error(ctx, "Value does not support member access");
}
cubec_value_t cubec_value_set_field(cubec_value_t self,
                                    struct _cubec_context_t *ctx,
                                    const char *name, cubec_value_t value) {
  cubec_type_t type = cubec_value_get_type(self);
  cubec_type_operator_t opt = cubec_type_get_operator(type);
  if (opt->set_field) {
    return opt->set_field(self, ctx, name, value);
  }
  return cubec_create_error(ctx, "Value does not support member access");
}
cubec_value_t cubec_value_get_length(cubec_value_t self,
                                     struct _cubec_context_t *ctx) {
  cubec_type_t type = cubec_value_get_type(self);
  cubec_type_operator_t opt = cubec_type_get_operator(type);
  if (opt->get_length) {
    return opt->get_length(self, ctx);
  }
  return cubec_create_error(ctx, "Value does not support get length");
}
cubec_value_t cubec_value_call(cubec_value_t self, cubec_context_t ctx,
                               size_t argc, cubec_value_t argv[]) {

  cubec_type_t type = cubec_value_get_type(self);
  cubec_type_operator_t opt = cubec_type_get_operator(type);
  if (opt->call) {
    return opt->call(self, ctx, argc, argv);
  }
  return cubec_create_error(ctx, "Value is not callable");
}